// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _QCOMTEE_OBJECT_H
#define _QCOMTEE_OBJECT_H

#include <stdarg.h>
#include <stdatomic.h>
#include "qcomtee_errno.h"

#include <stdio.h>
#define MSGV printf
#define MSGD printf
#define MSGE printf

typedef uint32_t qcomtee_op_t;

/* 'OBJECT FLAGS' */

/**
 * @defgroup ObjRefFlags Object's Flags
 * @brief Sets of flags for an object.
 * @{
 */

/**
  * @def QCOMTEE_OBJREF_TEE
  * @brief It indicates that the object is hosted in QTEE.
  */
#define QCOMTEE_OBJREF_TEE (1 << 0)

/**
 * @def QCOMTEE_OBJREF_USER
 * @brief It indicates that the object is hosted in userspace.
 */
#define QCOMTEE_OBJREF_USER (1 << 1)

/**
 * @def QCOMTEE_OBJREF_MEM
 * @brief It indicates that the object is a memory object.
 */
#define QCOMTEE_OBJREF_MEM (1 << 2)

/** @} */ // end of ObjRefFlags

/* 'RESERVED OPERATIONS' */

/**
 * @defgroup ObjRefOperations Object's Operations
 * @brief Sets of reserved operation each object should support.
 * @{
 */

/**
 * @def QCOMTEE_OBJREF_OP_RELEASE
 * @brief Release object.
 * 
 */
#define QCOMTEE_OBJREF_OP_RELEASE ((qcomtee_op_t)(65536))

/** @} */ // end of ObjRefOperations

/**
 * @defgroup QCOMTEEParamType QTEE Parameter types
 * @brief Types of parameters as understood by QTEE driver.
 * @{
 */

/**
 * @def QCOMTEE_UBUF_INPUT
 * @brief A Memory Buffer allocated by the user for sharing data with QTEE
 *        for the duration of the object invocation and is tagged as input.
 */
#define QCOMTEE_UBUF_INPUT 0x00000008

/**
 * @def QCOMTEE_UBUF_OUTPUT
 * @brief A Memory Buffer allocated by the user for sharing data with QTEE
 *        for the duration of the object invocation and is tagged as output.
 */
#define QCOMTEE_UBUF_OUTPUT 0x00000009

/**
 * @def QCOMTEE_OBJREF_INPUT
 * @brief An Object Reference tagged as input.
 *
 * This can be an object sent to QTEE as an input parameter to request a
 * service or an object that is sent back by QTEE as an input parameter in
 * a callback request.
 */
#define QCOMTEE_OBJREF_INPUT 0x0000000B

/**
 * @def QCOMTEE_OBJREF_OUTPUT
 * @brief An Object Reference tagged as input.
 *
 * This can be an object sent to QTEE as an output parameter resulting from
 * a callback request, or an object sent back by QTEE as a result of a request.
 */
#define QCOMTEE_OBJREF_OUTPUT 0x0000000C

/** @} */ // end of QCOMTEEParamType

struct qcomtee_object;

/**
 * @brief User buffer to send or receive data with QTEE.
 * 
 * This buffer is not shared with QTEE; the contents are copied back and
 * forth into a shared buffer.
 */
struct qcomtee_ubuf {
	void *addr; /**< Buffer address. */
	size_t size; /**< size of buffer in bytes. */
};

/**
 * @brief Parameter as passed to/from QTEE.
 */
struct qcomtee_param {
	uint64_t attr; /** < Type of parameter as in @ref QCOMTEEParamType. */
	union {
		struct qcomtee_ubuf ubuf; /**< User buffer if parameter type is
					       @ref QCOMTEEParamType::QCOMTEE_UBUF_INPUT or
					       @ref QCOMTEEParamType::QCOMTEE_UBUF_OUTPUT.*/
		struct qcomtee_object *object; /**< Object if parameter type is
						    @ref QCOMTEEParamType::QCOMTEE_OBJREF_INPUT or
						    @ref QCOMTEEParamType::QCOMTEE_OBJREF_OUTPUT.*/
	};
};

/**
 * @brief It is a special object representing NULL.
 */
#define QCOMTEE_OBJECT_NULL ((struct qcomtee_object *)(NULL))

/**
 * @brief Types of @ref qcomtee_object.
 */
typedef enum {
	QCOMTEE_OBJECT_TYPE_NULL, /**< It is a NULL object. */
	QCOMTEE_OBJECT_TYPE_TEE, /**< It is an object hosted in QTEE. */
	QCOMTEE_OBJECT_TYPE_ROOT, /**< It is a root object. */
	QCOMTEE_OBJECT_TYPE_CB, /**< It is a callback object. */
	QCOMTEE_OBJECT_TYPE_MEMORY, /**< It is a memory object. */
} qcomtee_object_type_t;

/**
 * @def QCOMTEE_OBJECT_PARAMS_MAX
 * @brief Size of parameter array minus one passed to the dispatcher.
 *
 * This is allocated on the stack; use your best judgment.
 */
#define QCOMTEE_OBJECT_PARAMS_MAX 10

/**
 * @brief Object's operations.
 * 
 * The client implementing these operations should serialize multiple
 * concurrent dispatch calls.
 */
struct qcomtee_object_ops {
	/**
	 * @brief Release the object.
	 * @param object The object being released.
	 */
	void (*release)(struct qcomtee_object *object);

	/**
	 * @brief Dispatch an invocation for the object.
	 * @param[in] object Object being invoked by QTEE.
	 * @param[in] op Operation requested by QTEE.
	 * @param[in,out] params Parameter array for the invocation.
	 * @param[in] num Number of parameters in the params array.
	 *
	 * @return On success returns @ref QCOMTEE_OK;
	 *         Otherwise a number; see @ref qcomtee_errno.h. 
	 */
	qcomtee_result_t (*dispatch)(struct qcomtee_object *object,
				     qcomtee_op_t op,
				     struct qcomtee_param *params, int num);

	/**
	 * @brief Notify the object of the transport state.
	 *
	 * After a call to @ref qcomtee_object_ops::dispatch "Dispatcher",
	 * this is called to inform the object whether there was an issue in
	 * submitting the response, so that on error it can clean up.
	 *
	 * @param object The object being notified.
	 * @param err State of transport (0 is success; Otherwise error)
	 */
	void (*error)(struct qcomtee_object *object, int err);

	/**
	 * @brief Check if requested operation is supported.
	 * @param op Operation.
	 * @return If supported a non-zero value; Otherwise 0.
	 */
	int (*supported)(qcomtee_op_t op);
};

/**
 * @brief Object.
 * 
 * This represents a generic object, independent of where it is hosted.
 */
struct qcomtee_object {
	atomic_int refs; /**< Number of references to this object. */
	uint64_t object_id; /**< ID assigned to this object. */
	uint64_t tee_object_id; /**< ID assigned to this object for QTEE. */
	qcomtee_object_type_t object_type; /**< Object Type. */

	/**
	 * @brief It is non-zero if the object has already been exported to QTEE;
	 * For @ref qcomtee_object_type_t::QCOMTEE_OBJECT_TYPE_CB "Callback Object".
	 * 
	 * For callback objects, @ref qcomtee_object::object_id "object_id" remains
	 * invalid unless it is sent to the QTEE 'Deferred ID allocation'. When
	 * allocated, the object holds onto the ID until it is released, even
	 * if it is not being referenced by QTEE.
	 */
	int queued;

	/**
	 * @brief It is the root object to which this object belongs;
	 * For @ref qcomtee_object_type_t::QCOMTEE_OBJECT_TYPE_TEE "QTEE Object",
	 * for @ref qcomtee_object_type_t::QCOMTEE_OBJECT_TYPE_CB "Callback Object".
	 * For @ref qcomtee_object_type_t::QCOMTEE_OBJECT_TYPE_ROOT "Root Object",
	 * it points to itself.
	 * 
	 * The root for the callback object is set in @ref qcomtee_object_cb_init.
	 * A callback object can only be sent or received using a QTEE object
	 * with the same root. The root for the QTEE objects is inherited from
	 * the original object that returned the object.
	 */
	struct qcomtee_object *root;

	struct qcomtee_object_ops *ops; /**< Object callback operations. */
};

#define container_of(ptr, type, member) \
	((type *)((void *)(ptr) - __builtin_offsetof(type, member)))

/**
 * @brief Get type of object
 * @param object The object to check for its type.
 * @return Returns @ref qcomtee_object_type_t.
 */
static inline qcomtee_object_type_t
qcomtee_object_typeof(struct qcomtee_object *object)
{
	if (object == QCOMTEE_OBJECT_NULL)
		return QCOMTEE_OBJECT_TYPE_NULL;
	return object->object_type;
}

/**
 * @brief Increase object reference count.
 * @param object The object being incremented.
 * @return On success, returns 0;
 *         Otherwise, returns -1.
 */
static inline int qcomtee_object_refs_inc(struct qcomtee_object *object)
{
	int old;

	if (object == QCOMTEE_OBJECT_NULL)
		return -1;

	old = atomic_load(&object->refs);
	do {
		if (old == 0)
			return -1;
	} while (!atomic_compare_exchange_weak(&object->refs, &old, old + 1));

	return 0;
}

/**
 * @brief Decrease object reference count.
 * @param object The object being decremented.
 */
void qcomtee_object_refs_dec(struct qcomtee_object *object);

#ifdef __GLIBC__
typedef int (*tee_call_t)(int, unsigned long, ...);
#else
typedef int (*tee_call_t)(int, int, ...);
#endif

/**
 * @brief Create a root object.
 * @param dev TEE device file pathname.
 * @param tee_call API to call to TEE driver (e.g. ioctl()).
 * @param release Called on destruction of this root object.
 * @param arg Argument passed to release.
 *
 * It creates a new namespace. To release a root object,
 * use @ref qcomtee_object_refs_dec.
 * 
 * @return On success, returns the object;
 *         Otherwise, returns @ref QCOMTEE_OBJECT_NULL.
 */
struct qcomtee_object *qcomtee_object_root_init(const char *dev,
						tee_call_t tee_call,
						void (*release)(void *),
						void *arg);

/**
 * @brief Initialize a callback objet.
 * @param object Object to initialize as callback object.
 * @param ops Sets of callback operarion for the callback object.
 * @param root The root object to which this object belongs.
 * @return On success, 0; Otherwise, returns -1.
 */
int qcomtee_object_cb_init(struct qcomtee_object *object,
			   struct qcomtee_object_ops *ops,
			   struct qcomtee_object *root);

/**
 * @brief Invoke an Object.
 *
 * On success, the caller loses ownership of input callback objects and
 * should wait for QTEE to release them.
 *
 * @param object Object to invoke.
 * @param op Operation to do on the object.
 * @param params Input parameter array to the requested operation.
 * @param num_params Number of parameter in the input array.
 * @param result Result of operation.
 * @return On success, 0; Otherwise, returns -1.
 */
int qcomtee_object_invoke(struct qcomtee_object *object, qcomtee_op_t op,
			  struct qcomtee_param *params, int num_params,
			  qcomtee_result_t *result);

/**
 * @brief Process single request.
 *
 * If the queue is empty, the caller will be blocked.
 * The success or failure of this function is not related to the state of
 * the actual request being processed.
 * 
 * This function calls the random object's @ref qcomtee_object_ops::dispatch
 * operation. Therefore, if it is being executed by a pthread, it is not
 * safe to use PTHREAD_CANCEL_ASYNCHRONOUS.
 *
 * The @ref qcomtee_object::tee_call function should implement support for
 * reading a new request (i.e., TEE_IOC_SUPPL_RECV) and submitting the response
 * (i.e., TEE_IOC_SUPPL_SEND).
 * 
 * @param root The root object for which the request queue is checked.
 * @return On success, 0; Otherwise, returns -1.
 */
int qcomtee_object_process_one(struct qcomtee_object *root);

#endif // _QCOMTEE_OBJECT_H
