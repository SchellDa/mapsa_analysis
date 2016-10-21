
#include "core.h"
#include "mpastreamreader.h"
#include "cbcstreamreader.h"

namespace core {

void initClasses()
{
	REGISTER_PIXEL_STREAM_READER_TYPE(MPAStreamReader);
	REGISTER_PIXEL_STREAM_READER_TYPE(CBCStreamReader);
}

} // namespace core
