#include <CommonCanReceiver.h>

namespace Can
{
bool CommonCanReceiver::setInterface(const std::string &interface)
{
	mInterface = interface;

	return true;
}

bool CommonCanReceiver::setFilters(std::set<CanFilter> filters)
{
	mFilters = filters;

	return true;
}

bool CommonCanReceiver::filter(u32 id)
{
	bool filtered = false;

	if (mFilters.empty())
		return true; // If no filters set, send everything

	for (auto filter = mFilters.begin(); filter != mFilters.end(); ++filter) {
		if ((filter->getId() & filter->getMask()) == (id & filter->getMask())) {
			filtered = true;
			break;
		}
	}

	return filtered;
}

} // namespace Can
