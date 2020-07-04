// forwards
void fexthunks_forward_fexthunk_test(void *argsv){
struct arg_t {int a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_fexthunk_test
(args->a_0);
}
void fexthunks_forward_fexthunk_test_void(void *argsv){
struct arg_t {int a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_fexthunk_test_void
(args->a_0);
}
void fexthunks_forward_fexthunk_add(void *argsv){
struct arg_t {int a_0;int a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_fexthunk_add
(args->a_0,args->a_1);
}
