
#include "datastructures.h"
#include <math.h>

ClassImp(Conditionals)
ClassImp(RippleCounter)
ClassImp(MemoryNoProcessing)
ClassImp(MpaData)
ClassImp(TelescopePlaneClusters)
ClassImp(PlaneHits)
ClassImp(TelescopeData)
ClassImp(TelescopeHits)

Conditionals::Conditionals() :
 threshold(NAN), voltage(NAN), current(NAN),
 temperature(NAN), z_pos(NAN), angle(NAN)
{
}

Conditionals::~Conditionals()
{
}

RippleCounter::RippleCounter() :
 header(0)
{
}

RippleCounter::~RippleCounter()
{
}

MemoryNoProcessing::MemoryNoProcessing() :
 numEvents(0), corrupt(0)
{
}

MemoryNoProcessing::~MemoryNoProcessing()
{
}

MpaData::MpaData() :
 counter(), noProcessing()
{
}

MpaData::~MpaData()
{
}

TelescopePlaneClusters::TelescopePlaneClusters()
 : x(), y()
{
}

TelescopePlaneClusters::~TelescopePlaneClusters()
{
}

TelescopeData::TelescopeData()
{
}

TelescopeData::~TelescopeData()
{
}

PlaneHits::PlaneHits()
{
}

PlaneHits::~PlaneHits()
{
}

TelescopeHits::TelescopeHits()
{
}

TelescopeHits::~TelescopeHits()
{
}

