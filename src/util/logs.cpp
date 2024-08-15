#include "util/logs.hpp"

using namespace balatrogether;

std::mutex logger::stream::mutex;
logger::stream logger::info("[INFO] ", std::cout, "0");
logger::stream logger::debug("[DEBUG] ", std::cout, "33");
logger::stream logger::error("[ERROR] ", std::cerr, "31");