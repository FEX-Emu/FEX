// thunks
MAKE_THUNK(fexthunk, test)

int test(int a_0){
struct {int a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_fexthunk_test(&args);
return args.rv;
}

MAKE_THUNK(fexthunk, test_void)

void test_void(int a_0){
struct {int a_0;} args;
args.a_0 = a_0;
fexthunks_fexthunk_test_void(&args);
}

MAKE_THUNK(fexthunk, add)

int add(int a_0,int a_1){
struct {int a_0;int a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_fexthunk_add(&args);
return args.rv;
}

