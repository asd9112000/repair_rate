#include "../inc/V2GroupNoScratchPolicy.hpp"

namespace dynamic_spare
{
const char *toString(V2GroupAction action) noexcept
{
    switch (action)
    {
        case V2GroupAction::Local: return "LOCAL";
        case V2GroupAction::ReleaseOnly: return "RELEASE_ONLY";
        case V2GroupAction::BorrowOnly: return "BORROW_ONLY";
        case V2GroupAction::ReleaseAndBorrow: return "RELEASE_AND_BORROW";
    }
    return "UNKNOWN";
}
} // namespace dynamic_spare
