#!/bin/bash
iverilog -grelative-include -E ./modules/top.sv -o project.sv
