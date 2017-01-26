
#include "core.h"
#include "coreconfig.h"
#include "mpastreamreader.h"
#include "mpamemorystreamreader.h"
#include "cbcstreamreader.h"

namespace core {

void initClasses()
{
	REGISTER_PIXEL_STREAM_READER_TYPE(MPAStreamReader);
	REGISTER_PIXEL_STREAM_READER_TYPE(MpaMemoryStreamReader);
#ifdef ENABLE_CBC_ANALYIS
	REGISTER_PIXEL_STREAM_READER_TYPE(CBCStreamReader);
#endif//ENABLE_CBC_ANALYIS
}

} // namespace core
