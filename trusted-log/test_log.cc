#include "log.h"
#include <iostream>
#include <stdlib.h>

int main(void*) {
	std::unique_ptr<trusted_log> log = std::make_unique<trusted_log>(1024);
	trusted_log::log_entry e;
	log->append(e);
	std::cout << "#####\n";
	trusted_log log2(std::move(*log.get()));
	log2.append(e);
	std::cout <<log2.get_log_size() << "\n";
	trusted_log log3(trusted_log(2048));
	log3.append(e);
	std::cout <<log3.get_log_size() << "\n";
	return 0;
}
