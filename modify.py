import random
import mmap
import os
import sys
import struct
#76,77
#190,200

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

  #print 'XOR: ', h
  return h
  
def hashAdd(mm, block):
  h = 0
  start = block['endCheckers'] if block['noCC'] else block['startAddr']
  end = block['endAddr']
  #print start, end
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

  #print 'HASH: ', h
  return h
  
if len(sys.argv) != 2:
  print 'Incorrect usage. Expected: python modify.py <binary_file_name>'
  sys.exit(1)

with open(sys.argv[1], 'r+b') as f:
  mm = mmap.mmap(f.fileno(), 0)

  print 'Finding Dyninst Section Header'
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

  print 'Gathering all instrumented blocks'
  blocks = {}
  index = sectionFileOffset
  block = {}
  lastId = -1
  dups = False
  while index != -1:
    index = mm.find(b'\x48\xb8\x44\x33\x22\x11', index + 1, mm.size())
    if index != -1:
      # -7 to account for saving certain regs
      start = index - 7
      block['endAddr'] = start
      mm.seek(start + 13, os.SEEK_SET)
      blockId = struct.unpack('<L', mm.read(4))[0]
      if blockId not in blocks:
        lastId = blockId
      endCheckers = mm.find(b'\x48\xb8\x11\x22\x33\x44', start)
      block = { 'blockId': blockId, 'attachCheck': True, 'startAddr': start, 'endCheckers': endCheckers + 23, 'checkees': [], 'checkers': [] }
      while index != -1:
        index = mm.find(b'\x48\xb8\xcd\xab\xcd\xab', index + 1, endCheckers)
        if index != -1:
          block['checkees'].append(index)
      if blockId in blocks:
        block['attachCheck'] = False
        dups = True
      blocks[blockId] = block
      index = block['endCheckers']
  block['endAddr'] = sectionFileOffset + sectionSize # TODO: may this needs to be - 1?

  if dups and lastId != -1:
    print 'Found Duplicates, fixing "last" block end address'
    lastBlock = blocks[lastId]
    end = mm.find(b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00', lastBlock['startAddr'])
    blocks[lastId]['endAddr'] = end

  print 'Verifying Checker structure'
  for k in blocks:
    assert len(blocks[k]['checkees']) % 3 == 0

  print 'Constructing internal checker network'
  for b in blocks.values():
    for i in range(0, len(b['checkees']), 3):
      mm.seek(b['checkees'][i] + 6, os.SEEK_SET)
      tag = struct.unpack('<L', mm.read(4))[0]
      blockId = tag & 0x7FFFFFFF
      assert tag >= 0x80000000 or blockId < b['blockId']
      checkee = blocks[blockId]
      checkee['noCC'] = tag >= 0x80000000
      checkee['checkers'].append({'hash': b['checkees'][i], 'start': b['checkees'][i + 1], 'end': b['checkees'][i + 2]})
      checkee['jmpVirt'] = b['checkees'][i + 2] + 0x26
    
  #print 'Verifying internal checker network'
  #for k in blocks:
  #  assert len(blocks[k]['checkers']) == 1

  blocks = blocks.values()
  blocks = sorted(blocks, key=lambda block: block['blockId'])

  #for b in blocks:
  #  print b
  #sys.exit(1)

  print 'Patching checkers'
  # Walk through blocks, creating checkers and patching values
  for block in blocks:
    #if not block['attachCheck'] or block['noCC']:
    #  print 'skipping', block['blockId']
    #  continue
    ha = hashAdd(mm, block)
    hx = hashXOR(mm, block)
    start = (block['endCheckers'] if block['noCC'] else block['startAddr']) - sectionFileOffset + sectionVirtual
    for c in block['checkers']:
      response = mm.find(b'\x48\xb8\x78\x56\x34\x12', c['end'])
      if response != -1:
        mm.seek(response, os.SEEK_SET)
        mm.write(struct.pack('<L', 0x0f05b848))
        mm.write(struct.pack('<L', 0x000031ff))
        mm.write(struct.pack('<L', 0xba490000))
        mm.write(struct.pack('<Q', block['endCheckers'] - sectionFileOffset + sectionVirtual))
      else:
        print "cannot find response tag"

      # patch variables
      mm.seek(c['start'] + 2, os.SEEK_SET)
      mm.write(struct.pack('<Q', start))
      mm.seek(c['end'] + 2, os.SEEK_SET)
      mm.write(struct.pack('<Q', block['endAddr'] - sectionFileOffset + sectionVirtual))
      mm.seek(c['hash'] + 2, os.SEEK_SET)
      if random.randint(1, 2) == 1:
        # Addr
        mm.write(struct.pack('<Q', ha))
      else:
        # XOR
        mm.write(struct.pack('<Q', hx))
        addr = mm.find(b'\x4c\x03\xd3', c['end'])
        mm.seek(addr, os.SEEK_SET)
        mm.write(b'\x49\x31\xda')

  index = sectionFileOffset
  while index != -1:
    index = mm.find(b'\x48\xb8\xcd\xab\xcd\xab', index + 1)
    if index != -1:
      mm.seek(index + 2, os.SEEK_SET)
      mm.write(struct.pack('<Q', sectionFileOffset))
