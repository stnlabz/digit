/**
 * @file digit_policy.h
 * @brief Policy discovery and reading interface for STN-LABZ Digit Core.
 *
 * This interface defines the earliest policy-handling operations:
 *
 * - discovery of Markdown policy documents;
 * - bounded reading of a specific policy document.
 *
 * Policy interpretation, validation, approval verification, and activation
 * are intentionally outside this stage.
 */

#ifndef STN_LABZ_DIGIT_POLICY_H
#define STN_LABZ_DIGIT_POLICY_H

#include <stddef.h>

/**
 * @brief Maximum policy document size accepted during current Core development.
 *
 * One additional byte is reserved by callers for null termination.
 */
#define DIGIT_POLICY_DOCUMENT_MAX 65536U

/**
 * @brief Result of a policy-directory discovery operation.
 */
typedef struct digit_policy_discovery
{
    size_t markdown_files;
    size_t ignored_files;
} digit_policy_discovery_t;

/**
 * @brief Discovers Markdown policy documents in a local directory.
 *
 * This operation performs discovery only. It does not parse, approve,
 * validate, interpret, or activate policy documents.
 *
 * @param directory Local policy directory.
 * @param result Receives discovery counts.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_policy_discover(
    const char *directory,
    digit_policy_discovery_t *result
);

/**
 * @brief Reads a specific policy document into bounded caller memory.
 *
 * Reading a document does not establish that the document is valid,
 * approved, authoritative, or active.
 *
 * @param directory Policy directory.
 * @param filename Policy file name.
 * @param buffer Destination buffer.
 * @param capacity Destination buffer capacity.
 * @param bytes_read Receives the number of bytes read.
 *
 * @return 0 on success, non-zero on failure.
 */
int digit_policy_read(
    const char *directory,
    const char *filename,
    char *buffer,
    size_t capacity,
    size_t *bytes_read
);

#endif /* STN_LABZ_DIGIT_POLICY_H */