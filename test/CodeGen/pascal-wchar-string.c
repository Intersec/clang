// RUN: %clang_cc1 -emit-llvm -o -  %s -fpascal-strings -fshort-wchar  | FileCheck %s
// rdar://8020384

#include <stddef.h>

extern void abort (void);

typedef unsigned short UInt16;

typedef UInt16 UniChar;

int main(int argc, char* argv[])
{

        char st[] = "\pfoo";            // pascal string
        UniChar wt[] = L"\pbar";        // pascal Unicode string
	UniChar wt1[] = L"\p";
	UniChar wt2[] = L"\pgorf";

        if (st[0] != 3)
          abort ();
        if (wt[0] != 3)
          abort ();
        if (wt1[0] != 0)
          abort ();
        if (wt2[0] != 4)
          abort ();
        
        return 0;
}

// CHECK: c"\03\00b\00a\00r\00\00\00"
// CHECK: c"\04\00g\00o\00r\00f\00\00\00"


// PR8856 - -fshort-wchar makes wchar_t be unsigned.
// CHECK: @test2
// CHECK: store volatile i32 1, i32* %isUnsigned
void test2() {
  volatile int isUnsigned = (wchar_t)-1 > (wchar_t)0;
}
