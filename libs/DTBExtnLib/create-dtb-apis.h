// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

/** @file create-dtb-apis.h
  FdtLib (edk2) Extended APIs
**/

/**
 * fdt_add_tree - creates a new empty tree
 * @node: pointer to fdt_node_handle object
 * @size: size of buffer
 * 
 */
int fdt_add_tree(fdt_node_handle *node, int size);

/**
 * fdt_add_node - creates a new node
 * @node: pointer to fdt_node_handle object
 * @path: name of the subnode to locate
 * @offset: structure block offset of a node
 * 
 */
int fdt_add_node(fdt_node_handle *node, char *path, int *offset);

/**
 * fdt_add_prop_list_u8 - set property to u8 list
 * @node: pointer to fdt_node_handle object
 * @prop_name: name of the property to change
 * @val: 8-bit integer list value for the property
 * @count: size of list
 * 
 */
int fdt_add_prop_list_u8(fdt_node_handle *node, char *prop_name, uint8_t *val, uint32_t count);

/**
 * fdt_add_prop_list_u16 - set property to u16 list
 * @node: pointer to fdt_node_handle object
 * @prop_name: name of the property to change
 * @val: 16-bit integer list value for the property
 * @count: size of list
 * 
 */
int fdt_add_prop_list_u16(fdt_node_handle *node, char *prop_name, uint16_t *val, uint32_t count);

/**
 * fdt_add_prop_list_u32 - set property to u32 list
 * @node: pointer to fdt_node_handle object
 * @prop_name: name of the property to change
 * @val: 32-bit integer list value for the property
 * @count: size of list
 * 
 */
int fdt_add_prop_list_u32(fdt_node_handle *node, char *prop_name, uint32_t *val, uint32_t count);

/**
 * fdt_add_prop_list_u64 - set property to u64 list
 * @node: pointer to fdt_node_handle object
 * @prop_name: name of the property to change
 * @val: 64-bit integer list value for the property
 * @count: size of list
 * 
 */
int fdt_add_prop_list_u64(fdt_node_handle *node, char *prop_name, uint64_t *val, uint32_t count);

/**
 * fdt_add_prop_list_string - set property to string list
 * @node: pointer to fdt_node_handle object
 * @prop_name: name of the property to change
 * @val: string list value for the property
 * @count: size of list
 * 
 */
int fdt_add_prop_list_string(fdt_node_handle *node, char *prop_name, char *val, uint32_t count);
