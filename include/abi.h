#ifndef STNLABZ_MODULE_ABI_H
#define STNLABZ_MODULE_ABI_H

/*
 * STN-LABZ
 * Module ABI 1.4
 *
 * abi.h
 *
 * Reusable module lifecycle orchestration.
 */

#include "module.h"
#include "module_registry.h"


/*
 * Prepare a descriptor through:
 *
 *     DISCOVER -> VERIFY -> QUALIFY
 *
 * Qualification inventory restoration/persistence may
 * remain in the host integration layer.
 */
stnlabz_module_result_t
stnlabz_module_abi_prepare(
    stnlabz_module_registry_t *registry,
    const stnlabz_module_descriptor_t *descriptor
);


/*
 * Explicitly authorize, invoke start(), and commit the
 * ACTIVE registry transition.
 */
stnlabz_module_result_t
stnlabz_module_abi_authorize_and_activate(
    stnlabz_module_registry_t *registry,
    const char *module_id,
    const stnlabz_module_host_t *host
);


/*
 * ACTIVE -> STOPPED
 *
 * stop() executes first. Only a successful callback
 * permits the Core registry STOPPED transition.
 *
 * Qualification is retained.
 * Activation authority is cleared.
 */
stnlabz_module_result_t
stnlabz_module_abi_stop(
    stnlabz_module_registry_t *registry,
    const char *module_id
);


/*
 * STOPPED/non-running -> UNREGISTERED -> removed.
 *
 * After this succeeds, a platform loader may safely
 * release the corresponding shared library, provided
 * no host-owned references remain.
 */
stnlabz_module_result_t
stnlabz_module_abi_unregister(
    stnlabz_module_registry_t *registry,
    const char *module_id
);


/*
 * Convenience hot-replacement preparation boundary:
 *
 *     ACTIVE -> stop callback -> STOPPED
 *            -> UNREGISTERED -> removed
 *
 * This function does not load the replacement binary.
 * Platform discovery/loader code performs that step,
 * after which the new descriptor enters prepare().
 */
stnlabz_module_result_t
stnlabz_module_abi_prepare_replacement(
    stnlabz_module_registry_t *registry,
    const char *module_id
);


int
stnlabz_module_abi_self_test(void);


#endif
