#ifndef GCTBLOCKHEADERBASE_H_
#define GCTBLOCKHEADERBASE_H_

#include <vector>
#include <map>
#include <ostream>
#include <string>

/// Abstract base class for block headers.
class GctBlockHeaderBase
{
public:

  /// Construct with raw 32-bit header.
  GctBlockHeaderBase(const uint32_t data=0):d(data) {}
  
  /// Construct with four 8-bit values.  
  GctBlockHeaderBase(const unsigned char * data);
  
  /// Virtual destructor.
  virtual ~GctBlockHeaderBase() {};
  
  /// Returns true if it's valid block header
  bool valid() const { return ( blockLengthMap().find(this->id()) != blockLengthMap().end() ); }

  /// Get the raw header data
  uint32_t data() const { return d; }

  /// Get the block ID
  virtual unsigned int id() const = 0;

  /// Get the number of time samples
  virtual unsigned int nSamples() const = 0;
  
  /// Get the bunch crossing ID
  virtual unsigned int bcId() const = 0;

  /// Get the event ID
  virtual unsigned int eventId() const = 0;

  /// Get the fundamental block length (for 1 time sample)
  unsigned int length() const;

  /// Get the block name
  std::string name() const;

protected:

  // PROTECTED TYPEDEFS 
  typedef std::map<unsigned int, unsigned int> BlockLengthMap;
  typedef std::map<unsigned int, std::string> BlockNameMap;
 
  // PROTECTED MEMBER DATA
  uint32_t d; /// The header. Yes it really is protected data.

  // PROTECTED METHODS
  /// Pure virtual interface for accessing concrete-subclass static blocklength map.
  virtual BlockLengthMap& blockLengthMap() = 0;
  virtual const BlockLengthMap& blockLengthMap() const = 0;
  
  /// Pure virtual interface for accessing concrete-subclass static blockname map.
  virtual BlockNameMap& blockNameMap() = 0;
  virtual const BlockNameMap& blockNameMap() const = 0;
};

std::ostream& operator<<(std::ostream& os, const GctBlockHeaderBase& h);

#endif /*GCTBLOCKHEADERBASE_H_*/
