//===-- CGBlocks.h - state for LLVM CodeGen for blocks ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the internal state used for llvm translation for block literals.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_CODEGEN_CGBLOCKS_H
#define CLANG_CODEGEN_CGBLOCKS_H

#include "CodeGenTypes.h"
#include "clang/AST/Type.h"
#include "llvm/Module.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/AST/CharUnits.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprObjC.h"

#include "CGBuilder.h"
#include "CGCall.h"
#include "CGValue.h"

namespace llvm {
  class Module;
  class Constant;
  class Function;
  class GlobalValue;
  class TargetData;
  class FunctionType;
  class PointerType;
  class Value;
  class LLVMContext;
}

namespace clang {

namespace CodeGen {

class CodeGenModule;
class CGBlockInfo;

enum BlockFlag_t {
  BLOCK_HAS_COPY_DISPOSE =  (1 << 25),
  BLOCK_HAS_CXX_OBJ =       (1 << 26),
  BLOCK_IS_GLOBAL =         (1 << 28),
  BLOCK_USE_STRET =         (1 << 29),
  BLOCK_HAS_SIGNATURE  =    (1 << 30)
};
class BlockFlags {
  uint32_t flags;

  BlockFlags(uint32_t flags) : flags(flags) {}
public:
  BlockFlags() : flags(0) {}
  BlockFlags(BlockFlag_t flag) : flags(flag) {}

  uint32_t getBitMask() const { return flags; }
  bool empty() const { return flags == 0; }

  friend BlockFlags operator|(BlockFlags l, BlockFlags r) {
    return BlockFlags(l.flags | r.flags);
  }
  friend BlockFlags &operator|=(BlockFlags &l, BlockFlags r) {
    l.flags |= r.flags;
    return l;
  }
  friend bool operator&(BlockFlags l, BlockFlags r) {
    return (l.flags & r.flags);
  }
};
inline BlockFlags operator|(BlockFlag_t l, BlockFlag_t r) {
  return BlockFlags(l) | BlockFlags(r);
}

enum BlockFieldFlag_t {
  BLOCK_FIELD_IS_OBJECT   = 0x03,  /* id, NSObject, __attribute__((NSObject)),
                                    block, ... */
  BLOCK_FIELD_IS_BLOCK    = 0x07,  /* a block variable */

  BLOCK_FIELD_IS_BYREF    = 0x08,  /* the on stack structure holding the __block
                                    variable */
  BLOCK_FIELD_IS_WEAK     = 0x10,  /* declared __weak, only used in byref copy
                                    helpers */
  BLOCK_FIELD_IS_ARC      = 0x40,  /* field has ARC-specific semantics */
  BLOCK_BYREF_CALLER      = 128,   /* called from __block (byref) copy/dispose
                                      support routines */
  BLOCK_BYREF_CURRENT_MAX = 256
};

class BlockFieldFlags {
  uint32_t flags;

  BlockFieldFlags(uint32_t flags) : flags(flags) {}
public:
  BlockFieldFlags() : flags(0) {}
  BlockFieldFlags(BlockFieldFlag_t flag) : flags(flag) {}

  uint32_t getBitMask() const { return flags; }
  bool empty() const { return flags == 0; }

  /// Answers whether the flags indicate that this field is an object
  /// or block pointer that requires _Block_object_assign/dispose.
  bool isSpecialPointer() const { return flags & BLOCK_FIELD_IS_OBJECT; }

  friend BlockFieldFlags operator|(BlockFieldFlags l, BlockFieldFlags r) {
    return BlockFieldFlags(l.flags | r.flags);
  }
  friend BlockFieldFlags &operator|=(BlockFieldFlags &l, BlockFieldFlags r) {
    l.flags |= r.flags;
    return l;
  }
  friend bool operator&(BlockFieldFlags l, BlockFieldFlags r) {
    return (l.flags & r.flags);
  }
};
inline BlockFieldFlags operator|(BlockFieldFlag_t l, BlockFieldFlag_t r) {
  return BlockFieldFlags(l) | BlockFieldFlags(r);
}

/// CGBlockInfo - Information to generate a block literal.
class CGBlockInfo {
public:
  /// Name - The name of the block, kindof.
  const char *Name;

  /// The field index of 'this' within the block, if there is one.
  unsigned CXXThisIndex;

  class Capture {
    uintptr_t Data;

  public:
    bool isIndex() const { return (Data & 1) != 0; }
    bool isConstant() const { return !isIndex(); }
    unsigned getIndex() const { assert(isIndex()); return Data >> 1; }
    llvm::Value *getConstant() const {
      assert(isConstant());
      return reinterpret_cast<llvm::Value*>(Data);
    }

    static Capture makeIndex(unsigned index) {
      Capture v;
      v.Data = (index << 1) | 1;
      return v;
    }

    static Capture makeConstant(llvm::Value *value) {
      Capture v;
      v.Data = reinterpret_cast<uintptr_t>(value);
      return v;
    }    
  };

  /// The mapping of allocated indexes within the block.
  llvm::DenseMap<const VarDecl*, Capture> Captures;  

  /// CanBeGlobal - True if the block can be global, i.e. it has
  /// no non-constant captures.
  bool CanBeGlobal : 1;

  /// True if the block needs a custom copy or dispose function.
  bool NeedsCopyDispose : 1;

  /// HasCXXObject - True if the block's custom copy/dispose functions
  /// need to be run even in GC mode.
  bool HasCXXObject : 1;

  /// UsesStret : True if the block uses an stret return.  Mutable
  /// because it gets set later in the block-creation process.
  mutable bool UsesStret : 1;

  llvm::StructType *StructureType;
  const BlockExpr *Block;
  CharUnits BlockSize;
  CharUnits BlockAlign;

  const Capture &getCapture(const VarDecl *var) const {
    llvm::DenseMap<const VarDecl*, Capture>::const_iterator
      it = Captures.find(var);
    assert(it != Captures.end() && "no entry for variable!");
    return it->second;
  }

  const BlockDecl *getBlockDecl() const { return Block->getBlockDecl(); }
  const BlockExpr *getBlockExpr() const { return Block; }

  CGBlockInfo(const BlockExpr *blockExpr, const char *Name);
};

}  // end namespace CodeGen
}  // end namespace clang

#endif
