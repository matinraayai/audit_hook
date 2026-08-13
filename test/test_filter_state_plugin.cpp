#include "audit_hook.hpp"

int dummy_target() { return 0; }

__attribute__((constructor)) void init() {
    const char* libA[] = {"libA.so"};
    const char* libB[] = {"libB.so"};
    const char* libAB[] = {"libA.so", "libB.so"};

    // Test 1: Include -> Exclude (Set Diff with Warning)
    audit_hooks::register_replace<dummy_target>("tool1", "func1");
    ah_set_caller_filter("tool1", AH_FILTER_INCLUDE, libA);
    // libB is not in the active INCLUDE list, so this must trigger a warning
    ah_set_caller_filter("tool1", AH_FILTER_EXCLUDE, libB); 

    // Test 2: Exclude -> Include (Inverted Set Diff with Warning)
    audit_hooks::register_replace<dummy_target>("tool2", "func2");
    ah_set_caller_filter("tool2", AH_FILTER_EXCLUDE, libA);
    // libB is not in the active EXCLUDE list, so this must trigger a warning
    ah_set_caller_filter("tool2", AH_FILTER_INCLUDE, libB); 

    // Test 3: Include -> Exclude (Valid Set Diff, NO warning)
    audit_hooks::register_replace<dummy_target>("tool3", "func3");
    ah_set_caller_filter("tool3", AH_FILTER_INCLUDE, libAB);
    // libA is in the list, safely removed from INCLUDE
    ah_set_caller_filter("tool3", AH_FILTER_EXCLUDE, libA); 

    // Test 4: Exclude -> Include (Valid Inverted Set Diff, NO warning)
    audit_hooks::register_replace<dummy_target>("tool4", "func4");
    ah_set_caller_filter("tool4", AH_FILTER_EXCLUDE, libAB);
    // libA is in the list, safely removed from EXCLUDE
    ah_set_caller_filter("tool4", AH_FILTER_INCLUDE, libA); 
}