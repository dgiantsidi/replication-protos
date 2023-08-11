#!/bin/env sh

clang-format -i ./test/*cc ./test/*h
clang-format -i ./cr/*cc ./cr/*h
clang-format -i ./bft-cr/*cc ./bft-cr/*h
clang-format -i ./rf/*cc ./rf/*h
clang-format -i ./alc/*cc ./alc/*h
clang-format -i digital_signatures/*cc digital_signatures/*h
clang-format -i **/**/*cc **/**/*cpp **/**/*hpp **/**/*h
clang-format -i **/*cc **/*cpp **/*hpp **/*h
