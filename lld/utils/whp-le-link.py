#!/usr/bin/env python3
"""Experimental Windows 386 VxD LE linker for freestanding i386 COFF objects.

No implicit runtime, libraries, imports, exception/unwind handling, or PE fallback.
Emits direct-header, dynamically loadable LE VxDs; guest loading remains unverified.
"""
import argparse
from dataclasses import dataclass, field
from pathlib import Path
import struct
import sys

PAGE = 4096


class LinkError(Exception):
    pass


def require(condition, message):
    if not condition:
        raise LinkError(message)


def get(data, offset, size, label):
    require(0 <= offset <= len(data) and 0 <= size <= len(data) - offset,
            f"{label}: truncated COFF data")
    return data[offset:offset + size]


def u16(data, off, label):
    return struct.unpack('<H', get(data, off, 2, label))[0]


def u32(data, off, label):
    return struct.unpack('<I', get(data, off, 4, label))[0]


def put16(out, n):
    require(0 <= n <= 0xFFFF, '16-bit field overflow')
    out.extend(struct.pack('<H', n))


def put32(out, n):
    require(0 <= n <= 0xFFFFFFFF, '32-bit field overflow')
    out.extend(struct.pack('<I', n))


def write32(out, offset, n):
    require(0 <= n <= 0xFFFFFFFF, '32-bit field overflow')
    struct.pack_into('<I', out, offset, n)


def align(n, a):
    return (n + a - 1) // a * a


@dataclass
class Section:
    name: str
    group: str
    data: bytes
    size: int
    alignment: int
    relocations: list
    base: int = -1


@dataclass
class Symbol:
    name: str
    section: int
    value: int
    storage: int


@dataclass
class Input:
    path: Path
    sections: list
    symbols: dict
    includes: list


@dataclass
class Object:
    name: str
    flags: int
    data: bytearray = field(default_factory=bytearray)
    virtual_size: int = 0
    first_page: int = 0
    pages: int = 0


GROUPS = ('text', 'rdata', 'data', 'bss')
FLAGS = {'text': 0x2245, 'rdata': 0x2261,
         'data': 0x2263, 'bss': 0x2263}


def parse_coff(path):
    content = path.read_bytes()
    require(len(content) >= 20, f'{path}: truncated COFF header')
    require(u16(content, 0, 'machine') == 0x014c,
            f'{path}: not i386 COFF (bigobj/PE/other machines unsupported)')
    nsecs = u16(content, 2, 'number of sections')
    symptr = u32(content, 8, 'symbol table pointer')
    nsyms = u32(content, 12, 'symbol count')
    require(0 < nsecs <= 512, f'{path}: invalid section count')
    require(u16(content, 16, 'optional header') == 0,
            f'{path}: executable COFF not supported; require relocatable .obj')
    headers = get(content, 20, nsecs * 40, 'section headers')
    symbytes = get(content, symptr, nsyms * 18, 'symbols')
    string_offset = symptr + nsyms * 18
    string_size = u32(content, string_offset, 'string table length')
    require(string_size >= 4, f'{path}: invalid string table length')
    strings = get(content, string_offset, string_size, 'string table')

    def name(raw):
        if raw[:4] == b'\0' * 4:
            pos = int.from_bytes(raw[4:8], 'little')
            require(4 <= pos < len(strings), f'{path}: invalid symbol name offset')
            end = strings.find(b'\0', pos)
            require(end >= 0, f'{path}: unterminated symbol')
            return strings[pos:end].decode('ascii')
        if raw[:1] == b'/':
            try:
                pos = int(raw[1:].split(b'\0')[0])
            except ValueError as error:
                raise LinkError(f'{path}: malformed long section name') from error
            require(4 <= pos < len(strings), f'{path}: bad section name offset')
            end = strings.find(b'\0', pos)
            require(end >= 0, f'{path}: unterminated section name')
            return strings[pos:end].decode('ascii')
        return raw.rstrip(b'\0').decode('ascii')

    sections = []
    includes = []
    for i in range(nsecs):
        header = headers[i * 40:(i + 1) * 40]
        section_name = name(header[:8])
        virtual_size = u32(header, 8, 'section virtual size')
        raw_size = u32(header, 16, 'section raw size')
        raw_ptr = u32(header, 20, 'section raw pointer')
        reloc_ptr = u32(header, 24, 'reloc pointer')
        nreloc = u16(header, 32, 'relocation count')
        attrs = u32(header, 36, 'section characteristics')
        if section_name.startswith('.debug') or section_name in ('.llvm_addrsig', '.drectve'):
            require(nreloc == 0, f'{path}: ignored section has relocations')
            if section_name == '.drectve':
                directive = get(content, raw_ptr, raw_size, 'linker directives').decode('ascii').strip()
                words = directive.split()
                require(all(w.upper().startswith('/INCLUDE:') for w in words),
                        f'{path}: unsupported .drectve option: {directive}')
                includes.extend(w.split(':', 1)[1] for w in words)
            sections.append(None)
            continue
        if attrs & 0x1000 or attrs & 0x800:
            raise LinkError(f'{path}: unsupported COMDAT/removed section {section_name}')
        lower = section_name.lower()
        group = next((g for g in GROUPS
                      if lower == '.' + g or lower.startswith('.' + g + '$')), None)
        require(group is not None, f'{path}: unsupported COFF section {section_name}')
        a = (attrs >> 20) & 0xF
        require(0 <= a <= 13, f'{path}: invalid alignment')
        alignment = (1 << (a - 1)) if a else 1
        require(alignment <= PAGE, f'{path}: alignment exceeds page size')
        is_bss = group == 'bss'
        require(not (is_bss and nreloc), f'{path}: relocations in uninitialized BSS')
        raw = get(content, raw_ptr, raw_size, f'{section_name} data') if raw_size and not is_bss else b''
        size = max(raw_size, virtual_size)
        require(size < (1 << 27), f'{path}: section too large')
        relocations = []
        if nreloc:
            records = get(content, reloc_ptr, 10 * nreloc, 'COFF relocations')
            for k in range(nreloc):
                o = 10 * k
                src, symbol, kind = struct.unpack_from('<IIH', records, o)
                require(src + 4 <= len(raw), f'{path}: relocation exceeds section')
                require(kind in (0x0006, 0x0014),
                        f'{path}: unsupported COFF i386 relocation type 0x{kind:04x}')
                relocations.append((src, symbol, kind))
        sections.append(Section(section_name, group, raw, size, alignment, relocations))

    symbols = {}
    i = 0
    while i < nsyms:
        raw = symbytes[i * 18:(i + 1) * 18]
        sym_name = name(raw[:8])
        value = u32(raw, 8, 'symbol value')
        section_number = struct.unpack_from('<h', raw, 12)[0]
        storage = raw[16]
        extras = raw[17]
        require(i + extras < nsyms, f'{path}: truncated COFF auxiliary symbols')
        symbols[i] = Symbol(sym_name, section_number, value, storage)
        i += 1 + extras
    return Input(path, sections, symbols, includes)


def resolve(source, index, globals_by_name):
    symbol = source.symbols.get(index)
    require(symbol is not None, f'{source.path}: relocation uses auxiliary/invalid symbol {index}')
    if symbol.section == 0:
        require(symbol.storage == 2 and symbol.value == 0,
                f'{source.path}: common/weak or special undefined symbol {symbol.name}')
        require(symbol.name in globals_by_name,
                f'{source.path}: undefined external {symbol.name}')
        source, symbol = globals_by_name[symbol.name]
    require(symbol.section > 0 and symbol.section <= len(source.sections),
            f'{source.path}: cannot relocate absolute/debug symbol {symbol.name}')
    target = source.sections[symbol.section - 1]
    require(target is not None, f'{source.path}: symbol in discarded section {symbol.name}')
    require(symbol.value <= target.size, f'{source.path}: invalid symbol offset {symbol.name}')
    return target, symbol.value


def link(inputs, name, ddb, vxd_id=0):
    require(1 <= len(name) <= 8 and name.isascii() and
            all(ch.isupper() or ch.isdigit() or ch == '_' for ch in name),
            'VxD resident name must be 1-8 uppercase ASCII characters')
    require(0 <= vxd_id <= 0xFFFF, 'VxD ID out of range')
    globals_by_name = {}
    for source in inputs:
        for sym in source.symbols.values():
            if sym.storage == 2 and sym.section > 0:
                require(sym.name not in globals_by_name,
                        f'duplicate global symbol {sym.name}')
                globals_by_name[sym.name] = source, sym
    if ddb not in globals_by_name and ('_' + ddb) in globals_by_name:
        ddb = '_' + ddb
    require(ddb in globals_by_name, f'export ordinal 1 DDB symbol not found: {ddb}')
    for source in inputs:
        for included in source.includes:
            require(included in globals_by_name,
                    f'{source.path}: /INCLUDE references missing symbol {included}')

    objects = []
    groups = {}
    for group in GROUPS:
        sections = [section for item in inputs for section in item.sections
                    if section is not None and section.group == group and section.size]
        if not sections:
            continue
        ob = Object(group, FLAGS[group])
        for section in sections:
            section.base = align(ob.virtual_size, section.alignment)
            ob.virtual_size = section.base + section.size
            require(ob.virtual_size <= 0x7FFFFFFF, 'LE object exceeds supported 2GiB')
            if len(ob.data) < section.base:
                ob.data.extend(b'\0' * (section.base - len(ob.data)))
            ob.data.extend(section.data)
            if section.size > len(section.data):
                ob.data.extend(b'\0' * (section.size - len(section.data)))
        groups[group] = len(objects) + 1
        objects.append(ob)
    require(objects, 'no allocatable COFF sections')

    def addr(source, symbol_index):
        sec, offset = resolve(source, symbol_index, globals_by_name)
        require(sec.group in groups, 'reference to zero-sized section')
        return groups[sec.group], sec.base + offset

    source, sym = globals_by_name[ddb]
    ddb_obj, ddb_offset = addr(source, next(index for index, candidate
                                           in source.symbols.items() if candidate is sym))
    require(ddb_offset < objects[ddb_obj - 1].virtual_size,
            'DDB must point inside a populated section')

    fixups = []
    for source in inputs:
        for sec in source.sections:
            if sec is None or sec.size == 0:
                continue
            ob_no = groups[sec.group]
            ob = objects[ob_no - 1]
            for src, symbol_index, kind in sec.relocations:
                abs_src = sec.base + src
                original = struct.unpack_from('<i', ob.data, abs_src)[0]
                target_obj, target_off = addr(source, symbol_index)
                target_off += original
                require(0 <= target_off < objects[target_obj - 1].virtual_size,
                        f'{source.path}: relocation target out of object range')
                require(abs_src % PAGE <= PAGE - 4,
                        f'{source.path}: cross-page relocation not supported')
                if kind == 0x0014 and ob_no == target_obj:
                    write32(ob.data, abs_src, (target_off - (abs_src + 4)) & 0xFFFFFFFF)
                else:
                    write32(ob.data, abs_src, 0)
                    fixups.append((ob_no, abs_src, kind, target_obj, target_off))

    output = bytearray(0x80 + 0xC4)
    output[:2] = b'MZ'
    struct.pack_into('<H', output, 0x02, 128)
    struct.pack_into('<H', output, 0x04, 1)
    struct.pack_into('<H', output, 0x08, 4)
    struct.pack_into('<H', output, 0x0E, 0)
    struct.pack_into('<H', output, 0x10, 0x100)
    struct.pack_into('<H', output, 0x18, 0x40)
    output[0x40:0x45] = b'\xB8\x01\x4C\xCD\x21'
    write32(output, 0x3C, 0x80)
    output[0x80:0x82] = b'LE'
    header = 0x80
    struct.pack_into('<HH', output, header + 8, 2, 4)
    write32(output, header + 0x10, 0x38000)
    write32(output, header + 0x28, PAGE)
    struct.pack_into('<HH', output, header + 0xC0, vxd_id, 0x0400)
    write32(output, header + 0x40, len(output) - header)
    write32(output, header + 0x44, len(objects))

    next_page = 1
    for ob in objects:
        ob.first_page = next_page
        ob.pages = align(ob.virtual_size, PAGE) // PAGE
        next_page += ob.pages
        for n in (ob.virtual_size, 0, ob.flags, ob.first_page, ob.pages, 0):
            put32(output, n)
    logical_pages = next_page - 1
    require(0 < logical_pages < (1 << 20), 'invalid logical page count')
    write32(output, header + 0x48, len(output) - header)
    physical_pages = []
    page_fixups = [[] for _ in range(logical_pages)]
    for src_obj, src_off, kind, tgt_obj, tgt_off in fixups:
        ob = objects[src_obj - 1]
        page_index = ob.first_page - 1 + src_off // PAGE
        mode = 0x1007 if kind == 0x0006 else 0x1008
        record = bytearray()
        put16(record, mode)
        put16(record, src_off % PAGE)
        require(tgt_obj < 256, 'more than 255 LE objects unsupported')
        record.append(tgt_obj)
        put32(record, tgt_off)
        page_fixups[page_index].append(bytes(record))
    page_index = 0
    for ob in objects:
        for page in range(ob.pages):
            contents = ob.data[page * PAGE:(page + 1) * PAGE]
            if any(contents) or page_fixups[page_index]:
                physical_pages.append(bytes(contents).ljust(PAGE, b'\0'))
                num = len(physical_pages)
                require(num <= 0xFFFFFF, 'too many physical LE pages')
                output.extend(num.to_bytes(3, 'big') + b'\0')
            else:
                output.extend(b'\0\0\0\3')
            page_index += 1
    write32(output, header + 0x14, len(physical_pages))
    write32(output, header + 0x2C, PAGE if physical_pages else 0)
    write32(output, header + 0x84, len(physical_pages))
    write32(output, header + 0x58, len(output) - header)
    output.append(len(name))
    output.extend(name.encode('ascii'))
    put16(output, 0)
    output.append(0)
    write32(output, header + 0x5C, len(output) - header)
    output.extend(b'\x01\x03')
    put16(output, ddb_obj)
    output.append(1)
    put32(output, ddb_offset)
    output.append(0)
    write32(output, header + 0x38, len(output) - (header + 0xC4))
    write32(output, header + 0x68, len(output) - header)
    record_offsets = [0]
    records = bytearray()
    for page in page_fixups:
        for rec in page:
            records.extend(rec)
        record_offsets.append(len(records))
    for value in record_offsets:
        put32(output, value)
    write32(output, header + 0x6C, len(output) - header)
    output.extend(records)
    write32(output, header + 0x30,
            len(output) - (header + u32(output, header + 0x68, 'fixup pages')))
    data_start = align(len(output), PAGE)
    write32(output, header + 0x80, data_start)
    output.extend(b'\0' * (data_start - len(output)))
    for page in physical_pages:
        output.extend(page)
    return bytes(output)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', required=True, help='Uppercase resident VxD name (1-8 chars)')
    parser.add_argument('--ddb', required=True, help='COFF symbol naming Device Descriptor Block')
    parser.add_argument('--vxd-id', type=lambda x: int(x, 0), default=0)
    parser.add_argument('-o', '--output', type=Path, required=True)
    parser.add_argument('inputs', type=Path, nargs='+', help='i386 COFF .obj files (not PE executables)')
    args = parser.parse_args(argv)
    try:
        objs = [parse_coff(path) for path in args.inputs]
        data = link(objs, args.name, args.ddb, args.vxd_id)
        args.output.write_bytes(data)
        print(f'LE VxD written: {args.output} ({len(data)} bytes); guest load NOT tested')
    except (OSError, LinkError, UnicodeError, struct.error) as exc:
        print(f'llvm-le-link: {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
