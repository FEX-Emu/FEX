// append ldflags: -z execstack
auto args = "stack, data_sym, text_sym";

#define EXECSTACK
#include "smc-1.inl"

/*
We cannot test the omagic or the static version of this, due to cross compiling issues
//#define OMAGIC // when the g++ driver is used to link, -Wl,--omagic breaks -static, so this can't be tested
#include "smc-1.inl"
*/
