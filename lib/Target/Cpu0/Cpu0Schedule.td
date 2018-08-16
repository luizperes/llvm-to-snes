//===-- Cpu0Schedule.td - Cpu0 Scheduling Definitions ------*- tablegen -*-===//
//
// The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//===----------------------------------------------------------------------===//
// Functional units across Cpu0 chips sets. Based on GCC/Cpu0 backend files.
//===----------------------------------------------------------------------===//
def ALU : FuncUnit;
def IMULDIV : FuncUnit;

//===----------------------------------------------------------------------===//
// Instruction Itinerary classes used for Cpu0
//===----------------------------------------------------------------------===//
def IIAlu : InstrItinClass;
def II_CLO : InstrItinClass;
def II_CLZ : InstrItinClass;
def IILoad : InstrItinClass;
def IIStore : InstrItinClass;
def IIBranch : InstrItinClass;
def IIPseudo : InstrItinClass;

//===----------------------------------------------------------------------===//
// Cpu0 Generic instruction itineraries.
//===----------------------------------------------------------------------===//
//@ http://llvm.org/docs/doxygen/html/structllvm_1_1InstrStage.html
def Cpu0GenericItineraries : ProcessorItineraries<[ALU, IMULDIV], [], [
  //@2
  InstrItinData<IIAlu , [InstrStage<1, [ALU]>]>,
  InstrItinData<II_CLO , [InstrStage<1, [ALU]>]>,
  InstrItinData<II_CLZ , [InstrStage<1, [ALU]>]>,
  InstrItinData<IILoad , [InstrStage<3, [ALU]>]>,
  InstrItinData<IIStore , [InstrStage<1, [ALU]>]>,
  InstrItinData<IIBranch , [InstrStage<1, [ALU]>]>
]>;
