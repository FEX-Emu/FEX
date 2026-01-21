# SPDX-License-Identifier: MIT

# This module just propagates a variable to parent scope.

macro(Propagate var)
    set(${var} ${${var}} PARENT_SCOPE)
endmacro()
