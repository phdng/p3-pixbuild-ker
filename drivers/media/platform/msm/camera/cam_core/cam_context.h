/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _CAM_CONTEXT_H_
#define _CAM_CONTEXT_H_

#include <linux/spinlock.h>
#include "cam_req_mgr_interface.h"
#include "cam_hw_mgr_intf.h"

/* Forward declarations */
struct cam_context;

/* max request number */
#define CAM_CTX_REQ_MAX              20

/**
 * enum cam_ctx_state -  context top level states
 *
 */
enum cam_context_state {
	CAM_CTX_UNINIT               = 0,
	CAM_CTX_AVAILABLE            = 1,
	CAM_CTX_ACQUIRED             = 2,
	CAM_CTX_READY                = 3,
	CAM_CTX_ACTIVATED            = 4,
	CAM_CTX_STATE_MAX            = 5,
};

/**
 * struct cam_ctx_request - Common request structure for the context
 *
 * @list:                  Link list entry
 * @status:                Request status
 * @request_id:            Request id
 * @req_priv:              Derived request object
 *
 */
struct cam_ctx_request {
	struct list_head   list;
	uint32_t           status;
	uint64_t           request_id;
	void              *req_priv;
};

/**
 * struct cam_ctx_ioctl_ops - Function table for handling IOCTL calls
 *
 * @acquire_dev:           Function pointer for acquire device
 * @release_dev:           Function pointer for release device
 * @config_dev:            Function pointer for config device
 * @start_dev:             Function pointer for start device
 * @stop_dev:              Function pointer for stop device
 *
 */
struct cam_ctx_ioctl_ops {
	int (*acquire_dev)(struct cam_context *ctx,
			struct cam_acquire_dev_cmd *cmd);
	int (*release_dev)(struct cam_context *ctx,
			struct cam_release_dev_cmd *cmd);
	int (*config_dev)(struct cam_context *ctx,
			struct cam_config_dev_cmd *cmd);
	int (*start_dev)(struct cam_context *ctx,
			struct cam_start_stop_dev_cmd *cmd);
	int (*stop_dev)(struct cam_context *ctx,
			struct cam_start_stop_dev_cmd *cmd);
};

/**
 * struct cam_ctx_crm_ops -  Function table for handling CRM to context calls
 *
 * @get_dev_info:          Get device informaiton
 * @link:                  Link the context
 * @unlink:                Unlink the context
 * @apply_req:             Apply setting for the context
 *
 */
struct cam_ctx_crm_ops {
	int (*get_dev_info)(struct cam_context *ctx,
			struct cam_req_mgr_device_info *);
	int (*link)(struct cam_context *ctx,
			struct cam_req_mgr_core_dev_link_setup *link);
	int (*unlink)(struct cam_context *ctx,
			struct cam_req_mgr_core_dev_link_setup *unlink);
	int (*apply_req)(struct cam_context *ctx,
			struct cam_req_mgr_apply_request *apply);
};


/**
 * struct cam_ctx_ops - Collection of the interface funciton tables
 *
 * @ioctl_ops:             Ioctl funciton table
 * @crm_ops:               CRM to context interface function table
 * @irq_ops:               Hardware event handle function
 *
 */
struct cam_ctx_ops {
	struct cam_ctx_ioctl_ops     ioctl_ops;
	struct cam_ctx_crm_ops       crm_ops;
	cam_hw_event_cb_func         irq_ops;
};

/**
 * struct cam_context - camera context object for the subdevice node
 *
 * @list:                  Link list entry
 * @sessoin_hdl:           Session handle
 * @dev_hdl:               Device handle
 * @link_hdl:              Link handle
 * @ctx_mutex:             Mutex for ioctl calls
 * @lock:                  Spin lock
 * @active_req_list:       Requests pending for done event
 * @pending_req_list:      Requests pending for reg upd event
 * @wait_req_list:         Requests waiting for apply
 * @free_req_list:         Requests that are free
 * @req_list:              Reference to the request storage
 * @req_size:              Size of the request storage
 * @hw_mgr_intf:           Context to HW interface
 * @ctx_crm_intf:          Context to CRM interface
 * @crm_ctx_intf:          CRM to context interface
 * @irq_cb_intf:           HW to context callback interface
 * @state:                 Current state for top level state machine
 * @state_machine:         Top level state machine
 * @ctx_priv:              Private context pointer
 *
 */
struct cam_context {
	struct list_head             list;
	int32_t                      session_hdl;
	int32_t                      dev_hdl;
	int32_t                      link_hdl;

	struct mutex                 ctx_mutex;
	spinlock_t                   lock;

	struct list_head             active_req_list;
	struct list_head             pending_req_list;
	struct list_head             wait_req_list;
	struct list_head             free_req_list;
	struct cam_ctx_request      *req_list;
	uint32_t                     req_size;

	struct cam_hw_mgr_intf      *hw_mgr_intf;
	struct cam_req_mgr_crm_cb   *ctx_crm_intf;
	struct cam_req_mgr_kmd_ops  *crm_ctx_intf;
	cam_hw_event_cb_func         irq_cb_intf;

	enum cam_context_state       state;
	struct cam_ctx_ops          *state_machine;

	void                        *ctx_priv;
};

/**
 * cam_context_handle_get_dev_info()
 *
 * @brief:        Handle get device information command
 *
 * @ctx:                   Object pointer for cam_context
 * @info:                  Device information returned
 *
 */
int cam_context_handle_get_dev_info(struct cam_context *ctx,
		struct cam_req_mgr_device_info *info);

/**
 * cam_context_handle_link()
 *
 * @brief:        Handle link command
 *
 * @ctx:                   Object pointer for cam_context
 * @link:                  Link command payload
 *
 */
int cam_context_handle_link(struct cam_context *ctx,
		struct cam_req_mgr_core_dev_link_setup *link);

/**
 * cam_context_handle_unlink()
 *
 * @brief:        Handle unlink command
 *
 * @ctx:                   Object pointer for cam_context
 * @unlink:                Unlink command payload
 *
 */
int cam_context_handle_unlink(struct cam_context *ctx,
		struct cam_req_mgr_core_dev_link_setup *unlink);

/**
 * cam_context_handle_apply_req()
 *
 * @brief:        Handle apply request command
 *
 * @ctx:                   Object pointer for cam_context
 * @apply:                 Apply request command payload
 *
 */
int cam_context_handle_apply_req(struct cam_context *ctx,
		struct cam_req_mgr_apply_request *apply);


/**
 * cam_context_handle_acquire_dev()
 *
 * @brief:        Handle acquire device command
 *
 * @ctx:                   Object pointer for cam_context
 * @cmd:                   Acquire device command payload
 *
 */
int cam_context_handle_acquire_dev(struct cam_context *ctx,
		struct cam_acquire_dev_cmd *cmd);

/**
 * cam_context_handle_release_dev()
 *
 * @brief:        Handle release device command
 *
 * @ctx:                   Object pointer for cam_context
 * @cmd:                   Release device command payload
 *
 */
int cam_context_handle_release_dev(struct cam_context *ctx,
		struct cam_release_dev_cmd *cmd);

/**
 * cam_context_handle_config_dev()
 *
 * @brief:        Handle config device command
 *
 * @ctx:                   Object pointer for cam_context
 * @cmd:                   Config device command payload
 *
 */
int cam_context_handle_config_dev(struct cam_context *ctx,
		struct cam_config_dev_cmd *cmd);

/**
 * cam_context_handle_start_dev()
 *
 * @brief:        Handle start device command
 *
 * @ctx:                   Object pointer for cam_context
 * @cmd:                   Start device command payload
 *
 */
int cam_context_handle_start_dev(struct cam_context *ctx,
		struct cam_start_stop_dev_cmd *cmd);

/**
 * cam_context_handle_stop_dev()
 *
 * @brief:        Handle stop device command
 *
 * @ctx:                   Object pointer for cam_context
 * @cmd:                   Stop device command payload
 *
 */
int cam_context_handle_stop_dev(struct cam_context *ctx,
		struct cam_start_stop_dev_cmd *cmd);

/**
 * cam_context_deinit()
 *
 * @brief:        Camera context deinitialize function
 *
 * @ctx:                   Object pointer for cam_context
 *
 */
int cam_context_deinit(struct cam_context *ctx);

/**
 * cam_context_init()
 *
 * @brief:        Camera context initialize function
 *
 * @ctx:                   Object pointer for cam_context
 * @crm_node_intf:         Function table for crm to context interface
 * @hw_mgr_intf:           Function table for context to hw interface
 * @req_list:              Requests storage
 * @req_size:              Size of the request storage
 *
 */
int cam_context_init(struct cam_context *ctx,
		struct cam_req_mgr_kmd_ops *crm_node_intf,
		struct cam_hw_mgr_intf *hw_mgr_intf,
		struct cam_ctx_request *req_list,
		uint32_t req_size);


#endif  /* _CAM_CONTEXT_H_ */