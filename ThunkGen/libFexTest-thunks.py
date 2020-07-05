#!/usr/bin/python3
from ThunkHelpers import *

lib("libFexTest")

fn("int test(int)")
fn("void test_void(int)")
fn("int add(int, int)")

Generate()
