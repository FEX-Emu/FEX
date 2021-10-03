#!/usr/bin/python3
from ThunkHelpers import *

lib("libxshmfence")
fn("int xshmfence_trigger(struct xshmfence *)")
fn("int xshmfence_await(struct xshmfence *)")
fn("int xshmfence_query(struct xshmfence *)")
fn("void xshmfence_reset(struct xshmfence *)")
fn("int xshmfence_alloc_shm()")
fn("struct xshmfence * xshmfence_map_shm(int)")
fn("void xshmfence_unmap_shm(struct xshmfence *)")
Generate()
