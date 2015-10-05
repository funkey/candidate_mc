#include "logging.h"

namespace pycmc {

logger::LogChannel pylog("pylog", "[pycmc] ");

logger::LogLevel getLogLevel() {
	return logger::LogManager::getGlobalLogLevel();
}

void setLogLevel(logger::LogLevel logLevel) {
	logger::LogManager::setGlobalLogLevel(logLevel);
}

} // namespace pycmc
