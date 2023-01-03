#include "logger.h"


ostreamFork::ostreamFork(std::ostream& os_one, std::ostream& os_two)
	: os1(os_one),
	os2(os_two)
{}
