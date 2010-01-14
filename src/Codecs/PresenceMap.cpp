// Copyright (c) 2009, Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#include <Common/QuickFASTPch.h>
#include "PresenceMap.h"
#include <Common/Exceptions.h>
#include <Common/Constants.h>
#include <Codecs/DataSource.h>
#include <Codecs/DataDestination.h>

using namespace ::QuickFAST;
using namespace ::QuickFAST::Codecs;

size_t
PresenceMap::maskToBitNumber(uchar bitMask)
{
  size_t bitNumber = 0;
  while(bitMask != PresenceMap::startByteMask &&  bitMask != 0)
  {
    bitNumber += 1;
    bitMask <<= 1;
  }
  return bitNumber;
}

PresenceMap::PresenceMap(size_t bits)
  : bitMask_(startByteMask)
  , bytePosition_(0)
  , byteCapacity_(defaultByteCapacity_)
  , bits_(&internalBuffer_[0])
  , vout_(0)
{
  size_t bytesNeeded = (bits + 6)/7;
  if(bytesNeeded > byteCapacity_)
  {
    byteCapacity_ = bytesNeeded;
    externalBuffer_.reset(new uchar[bytesNeeded]);
    bits_ = externalBuffer_.get();
  }
  std::fill(bits_, bits_ + byteCapacity_, '\0');
}
void
PresenceMap::setRaw(const uchar * buffer, size_t byteLength)
{
  if(byteLength > byteCapacity_)
  {
    byteCapacity_ = byteLength;
    externalBuffer_.reset(new uchar[byteCapacity_]);
    bits_ = externalBuffer_.get();
  }
  std::fill(bits_, bits_+byteCapacity_, 0);
  std::copy(buffer, buffer+byteLength, bits_);
  rewind();
}

void
PresenceMap::getRaw(const uchar *& buffer, size_t &byteLength)const
{
  buffer = &bits_[0];
  byteLength = byteCapacity_;
}

void
PresenceMap::grow()
{
  // todo: consider reporting this as a recoverable error due to performance impact
  boost::scoped_array<uchar> newBuffer(new uchar [byteCapacity_+1]);
  newBuffer[byteCapacity_] = 0;
  std::copy(bits_, bits_ + byteCapacity_, newBuffer.get());
  bits_ = newBuffer.get();
  externalBuffer_.swap(newBuffer);
  byteCapacity_ += 1;
}

void
PresenceMap::appendByte(size_t & pos, uchar byte)
{
  if(pos >= byteCapacity_)
  {
    assert(pos == byteCapacity_);
    grow();
  }
  bits_[pos++] = byte;
}

size_t
PresenceMap::encodeBytesNeeded()const
{
  // if no bits have been written
  if(bytePosition_ == 0 && bitMask_ == startByteMask)
  {
    return 0;
  }
  size_t bpos = bytePosition_;
  // if the last byte is unused, don't write it.
  if(bitMask_ == startByteMask)
  {
    bpos -= 1;
  }
  while(bpos > 0 && bits_[bpos] == 0)
  {
    bpos--;
  }
  return bpos + 1;
}

void
PresenceMap::encode(DataDestination & destination)
{
  if(bytePosition_ == 0 && bitMask_ == startByteMask)
  {
    return;
  }
  size_t bpos = bytePosition_;
  // if the last byte is unused, don't write it.
  if(bitMask_ == startByteMask)
  {
    bpos -= 1;
  }
  while(bpos > 0 && bits_[bpos] == 0)
  {
    bpos--;
  }
  bits_[bpos] |= stopBit;
  for(size_t pos = 0; pos <= bpos; ++pos)
  {
    destination.putByte(bits_[pos]);
  }
  if(vout_)
  {
    (*vout_) << "pmap["  <<  byteCapacity_ << "]->" << std::hex;
    for(size_t pos = 0; pos < byteCapacity_; ++pos)
    {
      (*vout_) << ' ' << std::setw(2) << static_cast<unsigned short>(bits_[pos]);
    }
    (*vout_) << " = ";
    for(size_t pos = 0; pos < byteCapacity_; ++pos)
    {
      uchar byte = bits_[pos];
      for(uchar mask = startByteMask; mask != 0; mask >>= 1)
      {
        (*vout_) << ((byte & mask) ? 'T' : 'f');
      }
    }

    (*vout_) << std::dec << std::endl;
  }
}

bool
PresenceMap::decode(Codecs::DataSource & source)
{
  reset();

  uchar byte;
  if(!source.getByte(byte))
  {
    return false;
  }
  size_t pos = 0;
  while((byte & stopBit) == 0)
  {
    appendByte(pos, byte);
    if(!source.getByte(byte))
    {
      return false;
    }
  }
  appendByte(pos, byte);

  if(vout_)
  {
    (*vout_) << "pmap["  <<  byteCapacity_ << "]<-" << std::hex;
    for(size_t pos = 0; pos < byteCapacity_; ++pos)
    {
      (*vout_) << ' ' << std::setw(2) <<  static_cast<unsigned short>(bits_[pos]);
    }
    (*vout_) << std::dec << std::endl;
  }

  return true;
}

void
PresenceMap::rewind()
{
  bytePosition_ = 0;
  bitMask_ = startByteMask;
}

void
PresenceMap::verboseCheckNextField(bool result)
{
  (*vout_) << "check pmap[" << bitNumber(bytePosition_, bitMask_) << " -> "
    << bytePosition_ << '/'
    << std::hex << static_cast<unsigned short>(bitMask_) << '&'
    << static_cast<unsigned short>(bits_[bytePosition_]) << std::dec << ']'
    << (result ? 'T' : 'F')
    << std::endl;
}


void
PresenceMap::verboseCheckSpecificField(size_t bit, size_t byte, uchar bitmask, bool result)
{
  (*vout_) << "check specific pmap[" << bit << " -> "
    << byte << '/'
    << std::hex << static_cast<unsigned short>(bitmask) << '&'
    << static_cast<unsigned short>(bits_[byte]) << std::dec << ']'
    << (result?'T' : 'F')
    << std::endl;
}

void
PresenceMap::reset(size_t bitCount)
{
  if(bitCount > 0)
  {
    size_t bytes = (bitCount + 7)/8;
    if(bytes > byteCapacity_)
    {
      externalBuffer_.reset(new uchar[bytes]);
      bits_ = externalBuffer_.get();
      byteCapacity_ = bytes;
    }
  }

  std::fill(bits_, bits_ + byteCapacity_, 0);
  rewind();

}
void
PresenceMap::verboseSetNext(bool present)
{
  (*vout_) << "set pmap[" << bitNumber(bytePosition_, bitMask_) << " -> "
    << bytePosition_ << '/'
    << std::hex << static_cast<unsigned short>(bitMask_) << std::dec << ']' <<(present?'T' : 'F')
    << std::endl;
}

bool
PresenceMap::operator == (const PresenceMap &  rhs)const
{
  if(bytePosition_ != rhs.bytePosition_) return false;
  if(bitMask_ != rhs.bitMask_) return false;
  for(size_t pos = 0; pos < bytePosition_; ++pos)
  {
    if(bits_[pos] != rhs.bits_[pos]) return false;
  }
  uchar mask = ((- bitMask_) << 1) & dataBits; // a bit of binary magic here ;-)
  if(((bits_[bytePosition_] ^ rhs.bits_[bytePosition_]) & mask) != 0) return false;
  return true;
}

