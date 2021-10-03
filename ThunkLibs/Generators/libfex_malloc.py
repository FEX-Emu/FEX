#!/usr/bin/python3
from ThunkHelpers import *

lib("libfex_malloc")

# FEX
fn("void fex_get_allocation_ptrs(AllocationPtrs *)")
no_ldr()

# Memory allocators

fn("void free(void*)"); no_unpack()
fn("void *calloc(size_t, size_t)"); no_unpack()
fn("void *memalign(size_t, size_t)"); no_unpack()
fn("void *realloc(void*, size_t)"); no_unpack()
fn("void *valloc(size_t)"); no_unpack()
fn("int posix_memalign(void**, size_t, size_t)"); no_unpack()
fn("void *aligned_alloc(size_t, size_t)"); no_unpack()
fn("size_t malloc_usable_size(void *)"); no_unpack()

fn("void *malloc(size_t)"); no_unpack()

Generate()
