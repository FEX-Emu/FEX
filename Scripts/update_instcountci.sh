#! /bin/bash
set -e

# Make sure we actually build
ninja

# Run tests, ignoring the retval since there will be changes.
ninja instcountci_tests || true

# Now we can update.
ninja instcountci_update_tests

# Commit the result in bulk.
git commit -sam "InstCountCI: Update"
