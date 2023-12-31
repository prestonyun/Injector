
#include <utility>
#include <cstdint>
#include <string>
#include <sys/types.h>

class Injector
{
public:
    static bool Inject(std::string module_path, std::int32_t pid) noexcept;
    static bool IsModuleLoaded(std::int32_t pid, const std::string& module_name);
};
