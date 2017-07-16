import random
import mmap
import os
import sys
import struct

def hashXOR(mm, block):
  h = 0
  start = block['endCheckers'] if block['noCC'] else block['startAddr']
  end = block['endAddr']
  # add check for jmp
  '''mm.seek(block['jmpReal'], os.SEEK_SET)
  h = h ^ struct.unpack('<L', mm.read(4))[0]
  mm.seek(block['jmpReal'] + 1, os.SEEK_SET)
  h = h ^ struct.unpack('<L', mm.read(4))[0]'''
  while start < end:
    mm.seek(start, os.SEEK_SET)
    val = struct.unpack('<L', mm.read(4))[0]
    h = h ^ val
    start += 1 

  print 'XOR: ', h
  return h
  
def hashAdd(mm, block):
  h = 0
  start = block['endCheckers'] if block['noCC'] else block['startAddr']
  end = block['endAddr']
  print start, end
  # add check for jmp
  '''mm.seek(block['jmpReal'], os.SEEK_SET)
  h = h + struct.unpack('<L', mm.read(4))[0]
  mm.seek(block['jmpReal'] + 1, os.SEEK_SET)
  h = h + struct.unpack('<L', mm.read(4))[0]'''
  while start < end:
    mm.seek(start, os.SEEK_SET)
    val = struct.unpack('<L', mm.read(4))[0]
    h = (h + val) % 0x10000000000000000
    start += 1 

  print 'HASH: ', h
  return h
  
if len(sys.argv) != 2:
  print 'Incorrect usage. Expected: python modify.py <binary_file_name>'
  sys.exit(1)

with open(sys.argv[1], 'r+b') as f:
  mm = mmap.mmap(f.fileno(), 0)

  mm.seek(0x3a, os.SEEK_SET)
  headerSize = struct.unpack('<H', mm.read(2))[0]

  # Find Dyninst Section Header
  mm.seek(-40, os.SEEK_END)
  shstrtab = struct.unpack('<Q', mm.read(8))[0]
  mm.seek(0, os.SEEK_SET)
  dyninstHeader = mm.size() -1
  while dyninstHeader != -1 and (mm.size() - dyninstHeader) % headerSize != 0:
    nameOffset = mm.find('.dyninstInst', shstrtab) - shstrtab
    dyninstHeader = mm.rfind(struct.pack('<I', nameOffset), 0, dyninstHeader)
  if dyninstHeader == -1:
    print 'Could not find dyninst header'
    sys.exit(1)

  # Find where Dyninst is in binary
  mm.seek(dyninstHeader + 16, os.SEEK_SET)
  sectionVirtual = struct.unpack('<Q', mm.read(8))[0]
  sectionFileOffset = struct.unpack('<Q', mm.read(8))[0]
  sectionSize = struct.unpack('<Q', mm.read(8))[0]

  blocks = {}
  index = sectionFileOffset
  block = {}
  while index != -1:
    index = mm.find(b'\x48\xb8\x44\x33\x22\x11', index + 1, mm.size())
    if index != -1:
      # -7 to account for saving certain regs
      start = index - 7
      block['endAddr'] = start
      mm.seek(start + 13, os.SEEK_SET)
      blockId = struct.unpack('<L', mm.read(4))[0]
      endCheckers = mm.find(b'\x48\xb8\x11\x22\x33\x44', start)
      block = { 'blockId': blockId, 'startAddr': start, 'endCheckers': endCheckers + 23, 'checkees': [], 'checkers': [] }
      while index != -1:
        index = mm.find(b'\x48\xb8\xcd\xab\xcd\xab', index + 1, endCheckers)
        if index != -1:
          block['checkees'].append(index)
      blocks[blockId] = block
      index = block['endCheckers']
  block['endAddr'] = sectionFileOffset + sectionSize # TODO: may this needs to be - 1?

  for k in blocks:
    assert len(blocks[k]['checkees']) % 3 == 0

  for b in blocks.values():
    for i in range(0, len(b['checkees']), 3):
      mm.seek(b['checkees'][i] + 6, os.SEEK_SET)
      tag = struct.unpack('<L', mm.read(4))[0]
      blockId = tag & 0x7FFFFFFF
      checkee = blocks[blockId]
      checkee['noCC'] = tag >= 0x80000000
      checkee['checkers'].append({'hash': b['checkees'][i], 'start': b['checkees'][i + 1], 'end': b['checkees'][i + 2]})
      checkee['jmpVirt'] = b['checkees'][i + 2] + 0x26
    
  for k in blocks:
    assert len(blocks[k]['checkers']) == 1
    

  """# Find all locations of checker placeholders
  locs = []
  index = sectionFileOffset
  while index != -1:
    index = mm.find(b'\x48\xb8\xcd\xab\xcd\xab', index + 1, mm.size())
    if index != -1:
      locs.append(index)
  
  
  # Find all locations of block placeholders
  blocklocs = []
  index = sectionFileOffset
  while index != -1:
    index = mm.find(b'\x48\xb8\x44\x33\x22\x11', index + 1, mm.size())
    if index != -1:
      # -7 to account for saving certain regs
      blocklocs.append(index - 7)
  

  # Decode Block locations
  blockInfos = []
  for l in blocklocs:
    mm.seek(l + 13, os.SEEK_SET)
    blockId = struct.unpack('<L', mm.read(4))[0]

    endCheckers = mm.find(b'\x48\xb8\x11\x22\x33\x44', l)

    if len(blockInfos) != 0:
      blockInfos[-1]['endAddr'] = l # TODO: may this needs to be - 1?
    blockInfos.append({ 'blockId': blockId, 'startAddr': l, 'endCheckers': endCheckers })
  blockInfos[-1]['endAddr'] = sectionFileOffset + sectionSize # TODO: may this needs to be - 1?
  
  #blockInfos = blockInfos[:-8]

  ids = {}
  for index, b in enumerate(blockInfos):
    if b['blockId'] in ids:
      print index, b['startAddr'] + sectionVirtual - sectionFileOffset, ids[b['blockId']]
    ids[b['blockId']] = b
  sys.exit(1)
  blockInfos = ids.values()

  # Group locs for same block together
  tags = []
  for l in locs:
    mm.seek(l + 6, os.SEEK_SET)
    tag = struct.unpack('<L', mm.read(4))[0]
    found = False
    for t in tags:
      if t['val'] == tag:
        t['locs'].append(l)
        found = True
    if not found:
      tags.append({ "val": tag, "locs": [l]})

  # Setup blocks and their checkers
  blocks = []
  for t in tags:
    block = { "blockId": t['val'] & 0x7FFFFFFF, "noCC": t['val'] >= 0x80000000, "checkers": []}
    blockInfo = None
    for b in blockInfos:
      if b['blockId'] == block['blockId']:
        blockInfo = b

    if blockInfo == None:
      print 'Could not find start of block'
      sys.exit(-1)

    block['startAddr'] = blockInfo['startAddr']
    block['endAddr'] = blockInfo['endAddr']
    block['endCheckers'] = blockInfo['endCheckers'] + 23

    # TODO: Addrs in group should always be 23 from eachother, add in check for this...
    for i in range(0, len(t['locs']), 3):
      block['checkers'].append({'hash': t['locs'][i], 'start': t['locs'][i + 1], 'end': t['locs'][i + 2] })
      block['jmpVirt'] = t['locs'][i + 2] + 0x26
      
    blocks.append(block)"""
  blocks = blocks.values()
  
  blocks = sorted(blocks, key=lambda block: block['blockId'])

  for b in blocks:
    print b
  #sys.exit(1)

  # Walk through blocks, creating checkers and patching values
  for block in blocks:
    ha = hashAdd(mm, block)
    hx = hashXOR(mm, block)
    start = (block['endCheckers'] if block['noCC'] else block['startAddr']) - sectionFileOffset + sectionVirtual
    print block['blockId']
    for c in block['checkers']:
      response = mm.find(b'\x48\xb8\x78\x56\x34\x12', c['end'])
      print '\t', c['end'], response
      mm.seek(response, os.SEEK_SET)
      mm.write(struct.pack('<L', 0x0f05b848))
      mm.write(struct.pack('<L', 0x000031ff))
      mm.write(struct.pack('<L', 0xba490000))
      mm.write(struct.pack('<Q', block['endCheckers'] - sectionFileOffset + sectionVirtual))
      

      mm.seek(c['start'] + 2, os.SEEK_SET)
      mm.write(struct.pack('<Q', start))
      mm.seek(c['end'] + 2, os.SEEK_SET)
      mm.write(struct.pack('<Q', block['endAddr'] - sectionFileOffset + sectionVirtual))
      mm.seek(c['hash'] + 2, os.SEEK_SET)
      if random.randint(1, 2) == 1:
        # Addr
        #print 'insert addr'
        mm.write(struct.pack('<Q', ha))
      else:
        # XOR
        print 'insert XOR'
        mm.write(struct.pack('<Q', hx))
        addr = mm.find(b'\x4c\x03\xd3', c['end'])
        print addr
        mm.seek(addr, os.SEEK_SET)
        mm.write(b'\x49\x31\xda')
