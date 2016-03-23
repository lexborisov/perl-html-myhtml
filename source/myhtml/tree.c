/*
 Copyright 2015 Alexander Borisov
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 
 Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#include "myhtml/tree.h"

myhtml_tree_t * myhtml_tree_create(void)
{
    return (myhtml_tree_t*)mycalloc(1, sizeof(myhtml_tree_t));
}

myhtml_status_t myhtml_tree_init(myhtml_tree_t* tree, myhtml_t* myhtml)
{
    myhtml_status_t status = MyHTML_STATUS_OK;
    
    tree->myhtml             = myhtml;
    tree->token              = myhtml_token_create(tree, 4096);
    tree->temp_tag_name.data = NULL;
    tree->queue              = myhtml->thread->queue;
    tree->single_queue       = NULL;
    tree->temp_stream        = NULL;
    
    tree->tree_obj = mcobject_async_create();
    if(tree->tree_obj == NULL)
        return MyHTML_STATUS_TREE_ERROR_MCOBJECT_CREATE;
    
    mcobject_async_status_t mcstatus = mcobject_async_init(tree->tree_obj, 128, 1024, sizeof(myhtml_tree_node_t));
    if(mcstatus)
        return MyHTML_STATUS_TREE_ERROR_MCOBJECT_INIT;
    
    tree->tags = myhtml_tag_create();
    status = myhtml_tag_init(tree, tree->tags);
    
    if(status) {
        myhtml->parse_state_func = NULL;
        myhtml->insertion_func = NULL;
        myhtml->thread = NULL;
        
        return status;
    }
    
    tree->mchar              = mchar_async_create(128, (4096 * 5));
    
    tree->indexes            = myhtml_tree_index_create(tree, tree->tags);
    tree->active_formatting  = myhtml_tree_active_formatting_init(tree);
    tree->open_elements      = myhtml_tree_open_elements_init(tree);
    tree->other_elements     = myhtml_tree_list_init();
    tree->token_list         = myhtml_tree_token_list_init();
    tree->template_insertion = myhtml_tree_template_insertion_init(tree);
    
    tree->mcasync_tree_id = mcobject_async_node_add(tree->tree_obj, &mcstatus);
    if(mcstatus)
        return MyHTML_STATUS_TREE_ERROR_MCOBJECT_CREATE_NODE;
    
    tree->mcasync_token_id = mcobject_async_node_add(tree->token->nodes_obj, &mcstatus);
    if(mcstatus)
        return MyHTML_STATUS_TREE_ERROR_MCOBJECT_CREATE_NODE;
    
    tree->mcasync_attr_id = mcobject_async_node_add(tree->token->attr_obj, &mcstatus);
    if(mcstatus)
        return MyHTML_STATUS_TREE_ERROR_MCOBJECT_CREATE_NODE;
    
    tree->mcasync_incoming_buf_id = mcobject_async_node_add(myhtml->async_incoming_buf, &mcstatus);
    if(mcstatus)
        return MyHTML_STATUS_TREE_ERROR_MCOBJECT_CREATE_NODE;
    
    tree->mchar_node_id = mchar_async_node_add(tree->mchar);
    
    // TODO: this code need send to some func
#ifndef MyHTML_BUILD_WITHOUT_THREADS
    
    tree->async_args = (myhtml_async_args_t*)calloc(myhtml->thread->pth_list_length, sizeof(myhtml_async_args_t));
    
    if(tree->async_args == NULL)
        return MyHTML_STATUS_TREE_ERROR_MEMORY_ALLOCATION;
    
    // for single mode in main thread
    tree->async_args[0].mchar_node_id = tree->mchar_node_id;
    
    // for batch thread
    for(size_t i = 0; i < myhtml->thread->batch_count; i++) {
        tree->async_args[(myhtml->thread->batch_first_id + i)].mchar_node_id = mchar_async_node_add(tree->mchar);
    }
    
#else /* MyHTML_BUILD_WITHOUT_THREADS */
    
    tree->async_args = (myhtml_async_args_t*)calloc(1, sizeof(myhtml_async_args_t));
    
    if(tree->async_args == NULL)
        return MyHTML_STATUS_TREE_ERROR_MEMORY_ALLOCATION;
    
    tree->async_args->mchar_node_id = tree->mchar_node_id;
    
#endif /* MyHTML_BUILD_WITHOUT_THREADS */
    
    tree->sync = mcsync_create();
    mcsync_init(tree->sync);
    
    myhtml_tree_clean(tree);
    
    return status;
}

void myhtml_tree_clean(myhtml_tree_t* tree)
{
    myhtml_t* myhtml = tree->myhtml;
    
#ifndef MyHTML_BUILD_WITHOUT_THREADS
    for(size_t i = 0; i < myhtml->thread->batch_count; i++) {
        mchar_async_node_clean(tree->mchar, tree->async_args[(myhtml->thread->batch_first_id + i)].mchar_node_id);
    }
#endif /* MyHTML_BUILD_WITHOUT_THREADS */
    
    mcobject_async_node_clean(tree->tree_obj, tree->mcasync_tree_id);
    mcobject_async_node_clean(tree->token->nodes_obj, tree->mcasync_token_id);
    mcobject_async_node_clean(tree->token->attr_obj, tree->mcasync_attr_id);
    mchar_async_node_clean(tree->mchar, tree->mchar_node_id);
    
    myhtml_token_clean(tree->token);
    
    // null root
    myhtml_tree_node_create(tree);
    
    tree->document  = myhtml_tree_node_create(tree);
    tree->fragment  = NULL;
    
    tree->doctype.is_html = false;
    tree->doctype.attr_name    = NULL;
    tree->doctype.attr_public  = NULL;
    tree->doctype.attr_system  = NULL;
    
    tree->node_html = 0;
    tree->node_body = 0;
    tree->node_head = 0;
    tree->node_form = 0;
    
    tree->state               = MyHTML_TOKENIZER_STATE_DATA;
    tree->state_of_builder    = MyHTML_TOKENIZER_STATE_DATA;
    tree->insert_mode         = MyHTML_INSERTION_MODE_INITIAL;
    tree->orig_insert_mode    = MyHTML_INSERTION_MODE_INITIAL;
    tree->compat_mode         = MyHTML_TREE_COMPAT_MODE_NO_QUIRKS;
    tree->tmp_tag_id          = MyHTML_TAG__UNDEF;
    tree->flags               = MyHTML_TREE_FLAGS_CLEAN|MyHTML_TREE_FLAGS_FRAMESET_OK;
    tree->foster_parenting    = false;
    tree->token_namespace     = NULL;
    tree->incoming_buf        = NULL;
    tree->incoming_buf_first  = NULL;
    tree->global_offset       = 0;
    tree->current_qnode       = NULL;
    tree->token_last_done     = NULL;
    
    tree->encoding            = MyHTML_ENCODING_UTF_8;
    tree->encoding_usereq     = MyHTML_ENCODING_DEFAULT;
    
    if(tree->single_queue)
        mythread_queue_clean(tree->single_queue);
    
    myhtml_tree_temp_stream_clean(tree);
    
    myhtml_tree_active_formatting_clean(tree);
    myhtml_tree_open_elements_clean(tree);
    myhtml_tree_list_clean(tree->other_elements);
    myhtml_tree_token_list_clean(tree->token_list);
    myhtml_tree_template_insertion_clean(tree);
    myhtml_tree_index_clean(tree, tree->tags);
    mcobject_async_node_clean(myhtml->async_incoming_buf, tree->mcasync_incoming_buf_id);
    myhtml_tag_clean(tree->tags);
    
    myhtml_token_attr_malloc(tree->token, tree->attr_current, tree->mcasync_attr_id);
}

void myhtml_tree_clean_all(myhtml_tree_t* tree)
{
    mcobject_async_clean(tree->tree_obj);
    myhtml_token_clean(tree->token);
    mchar_async_clean(tree->mchar);
    
    // null root
    myhtml_tree_node_create(tree);
    
    tree->document  = myhtml_tree_node_create(tree);
    tree->fragment  = NULL;
    
    tree->doctype.is_html = false;
    tree->doctype.attr_name    = NULL;
    tree->doctype.attr_public  = NULL;
    tree->doctype.attr_system  = NULL;
    
    tree->node_html = 0;
    tree->node_body = 0;
    tree->node_head = 0;
    tree->node_form = 0;
    
    tree->state               = MyHTML_TOKENIZER_STATE_DATA;
    tree->state_of_builder    = MyHTML_TOKENIZER_STATE_DATA;
    tree->insert_mode         = MyHTML_INSERTION_MODE_INITIAL;
    tree->orig_insert_mode    = MyHTML_INSERTION_MODE_INITIAL;
    tree->compat_mode         = MyHTML_TREE_COMPAT_MODE_NO_QUIRKS;
    tree->tmp_tag_id          = MyHTML_TAG__UNDEF;
    tree->flags               = MyHTML_TREE_FLAGS_CLEAN|MyHTML_TREE_FLAGS_FRAMESET_OK;
    tree->foster_parenting    = false;
    tree->token_namespace     = NULL;
    tree->incoming_buf        = NULL;
    tree->incoming_buf_first  = NULL;
    tree->global_offset       = 0;
    tree->current_qnode       = NULL;
    tree->token_last_done     = NULL;
    
    tree->encoding            = MyHTML_ENCODING_UTF_8;
    tree->encoding_usereq     = MyHTML_ENCODING_DEFAULT;
    
    if(tree->single_queue)
        mythread_queue_clean(tree->single_queue);
    
    myhtml_tree_temp_stream_clean(tree);
    
    myhtml_tree_active_formatting_clean(tree);
    myhtml_tree_open_elements_clean(tree);
    myhtml_tree_list_clean(tree->other_elements);
    myhtml_tree_token_list_clean(tree->token_list);
    myhtml_tree_template_insertion_clean(tree);
    myhtml_tree_index_clean(tree, tree->tags);
    mcobject_async_node_clean(tree->myhtml->async_incoming_buf, tree->mcasync_incoming_buf_id);
    myhtml_tag_clean(tree->tags);
    
    myhtml_token_attr_malloc(tree->token, tree->attr_current, tree->mcasync_attr_id);
}

myhtml_tree_t * myhtml_tree_destroy(myhtml_tree_t* tree)
{
    if(tree == NULL)
        return NULL;
    
    tree->active_formatting  = myhtml_tree_active_formatting_destroy(tree);
    tree->open_elements      = myhtml_tree_open_elements_destroy(tree);
    tree->other_elements     = myhtml_tree_list_destroy(tree->other_elements, true);
    tree->token_list         = myhtml_tree_token_list_destroy(tree->token_list, true);
    tree->template_insertion = myhtml_tree_template_insertion_destroy(tree);
    tree->indexes            = myhtml_tree_index_destroy(tree, tree->tags);
    tree->sync               = mcsync_destroy(tree->sync, true);
    tree->tree_obj           = mcobject_async_destroy(tree->tree_obj, true);
    tree->token              = myhtml_token_destroy(tree->token);
    tree->mchar              = mchar_async_destroy(tree->mchar, 1);
    tree->temp_stream        = myhtml_tree_temp_stream_free(tree);
    tree->tags               = myhtml_tag_destroy(tree->tags);
    
    if(tree->single_queue) {
        tree->single_queue = mythread_queue_destroy(tree->single_queue);
    }
    
    mcobject_async_node_delete(tree->myhtml->async_incoming_buf, tree->mcasync_incoming_buf_id);
    myhtml_tree_temp_tag_name_destroy(&tree->temp_tag_name, false);
    
    free(tree->async_args);
    free(tree);
    
    return NULL;
}

void myhtml_tree_node_clean(myhtml_tree_node_t* tree_node)
{
    tree_node->flags         = MyHTML_TREE_NODE_UNDEF;
    tree_node->tag_idx       = MyHTML_TAG__UNDEF;
    tree_node->prev          = 0;
    tree_node->next          = 0;
    tree_node->child         = 0;
    tree_node->parent        = 0;
    tree_node->last_child    = 0;
    tree_node->token         = 0;
    tree_node->my_namespace  = MyHTML_NAMESPACE_HTML;
}

myhtml_tree_indexes_t * myhtml_tree_index_create(myhtml_tree_t* tree, myhtml_tag_t* tags)
{
    myhtml_tree_indexes_t* indexes = (myhtml_tree_indexes_t*)mymalloc(sizeof(myhtml_tree_indexes_t));
    
    indexes->tags = myhtml_tag_index_create();
    myhtml_tag_index_init(tags, indexes->tags);
    
    return indexes;
}

void myhtml_tree_index_clean(myhtml_tree_t* tree, myhtml_tag_t* tags)
{
    if(tree->indexes == NULL)
        return;
    
    myhtml_tag_index_clean(tags, tree->indexes->tags);
}

myhtml_tree_indexes_t * myhtml_tree_index_destroy(myhtml_tree_t* tree, myhtml_tag_t* tags)
{
    if(tree->indexes == NULL)
        return NULL;
    
    tree->indexes->tags = myhtml_tag_index_destroy(tags, tree->indexes->tags);
    free(tree->indexes);
    
    return NULL;
}

void myhtml_tree_index_append(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    if(tree->indexes == NULL)
        return;
    
    myhtml_tag_index_add(tree->tags, tree->indexes->tags, node);
}

myhtml_tree_node_t * myhtml_tree_index_get(myhtml_tree_t* tree, myhtml_tag_id_t tag_id)
{
    if(tree->indexes == NULL)
        return NULL;
    
    myhtml_tag_index_node_t *tag_index = myhtml_tag_index_first(tree->indexes->tags, tag_id);
    
    if(tag_index)
        return tag_index->node;
    
    return NULL;
}

myhtml_t * myhtml_tree_get_myhtml(myhtml_tree_t* tree)
{
    if(tree)
        return tree->myhtml;
    
    return NULL;
}

myhtml_tag_t * myhtml_tree_get_tag(myhtml_tree_t* tree)
{
    if(tree)
        return tree->tags;
    
    return NULL;
}

myhtml_tag_index_t * myhtml_tree_get_tag_index(myhtml_tree_t* tree)
{
    if(tree && tree->indexes)
        return tree->indexes->tags;
    
    return NULL;
}

myhtml_tree_node_t * myhtml_tree_get_document(myhtml_tree_t* tree)
{
    return tree->document;
}

myhtml_tree_node_t * myhtml_tree_get_node_html(myhtml_tree_t* tree)
{
    return tree->node_html;
}

myhtml_tree_node_t * myhtml_tree_get_node_body(myhtml_tree_t* tree)
{
    return tree->node_body;
}

myhtml_tree_node_t * myhtml_tree_get_node_head(myhtml_tree_t* tree)
{
    return tree->node_head;
}

mchar_async_t * myhtml_tree_get_mchar(myhtml_tree_t* tree)
{
    return tree->mchar;
}

size_t myhtml_tree_get_mchar_node_id(myhtml_tree_t* tree)
{
    return tree->mchar_node_id;
}

myhtml_tree_node_t * myhtml_tree_node_create(myhtml_tree_t* tree)
{
    myhtml_tree_node_t* node = (myhtml_tree_node_t*)mcobject_async_malloc(tree->tree_obj, tree->mcasync_tree_id, NULL);
    myhtml_tree_node_clean(node);
    return node;
}

void myhtml_tree_node_add_child(myhtml_tree_t* tree, myhtml_tree_node_t* root, myhtml_tree_node_t* node)
{
    if(root->last_child) {
        root->last_child->next = node;
        node->prev = root->last_child;
    }
    else {
        root->child = node;
    }
    
    node->parent     = root;
    root->last_child = node;
}

void myhtml_tree_node_insert_before(myhtml_tree_t* tree, myhtml_tree_node_t* root, myhtml_tree_node_t* node)
{
    if(root->prev) {
        root->prev->next = node;
        node->prev = root->prev;
    }
    else {
        root->parent->child = node;
    }
    
    node->parent = root->parent;
    node->next   = root;
    root->prev   = node;
}

void myhtml_tree_node_insert_after(myhtml_tree_t* tree, myhtml_tree_node_t* root, myhtml_tree_node_t* node)
{
    if(root->next) {
        root->next->prev = node;
        node->next = root->next;
    }
    else {
        root->parent->last_child = node;
    }
    
    node->parent = root->parent;
    node->prev   = root;
    root->next   = node;
}

myhtml_tree_node_t * myhtml_tree_node_find_parent_by_tag_id(myhtml_tree_node_t* node, myhtml_tag_id_t tag_id)
{
    node = node->parent;
    
    while (node && node->tag_idx != tag_id) {
        node = node->parent;
    }
    
    return node;
}

myhtml_tree_node_t * myhtml_tree_node_remove(myhtml_tree_node_t* node)
{
    if(node->next)
        node->next->prev = node->prev;
    else if(node->parent)
        node->parent->last_child = node->prev;
    
    if(node->prev) {
        node->prev->next = node->next;
        node->prev = NULL;
    } else if(node->parent)
        node->parent->child = node->next;
    
    node->parent = NULL;
    
    if(node->next)
        node->next = NULL;
    
    return node;
}

void myhtml_tree_node_free(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    if(node == NULL)
        return;
    
    if(node->token) {
        myhtml_token_attr_delete_all(tree->token, node->token);
        myhtml_token_delete(tree->token, node->token);
    }
    
    mcobject_async_free(tree->tree_obj, node);
}

void myhtml_tree_node_delete(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    if(node == NULL)
        return;
    
    myhtml_tree_node_remove(node);
    myhtml_tree_node_free(tree, node);
}

void _myhtml_tree_node_delete_recursive(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    while(node)
    {
        if(node->child)
            _myhtml_tree_node_delete_recursive(tree, node->child);
        
        node = node->next;
        myhtml_tree_node_delete(tree, node);
    }
}

void myhtml_tree_node_delete_recursive(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    if(node == NULL)
        return;
    
    if(node->child)
        _myhtml_tree_node_delete_recursive(tree, node->child);
    
    myhtml_tree_node_delete(tree, node);
}

myhtml_tree_node_t * myhtml_tree_node_clone(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    myhtml_tree_node_t* new_node = myhtml_tree_node_create(tree);
    
    myhtml_token_node_wait_for_done(node->token);
    
    new_node->token            = myhtml_token_node_clone(tree->token, node->token, tree->mcasync_token_id, tree->mcasync_attr_id);
    new_node->tag_idx          = node->tag_idx;
    new_node->my_namespace     = node->my_namespace;
    new_node->token->type     |= MyHTML_TOKEN_TYPE_DONE;
    
    return new_node;
}

void myhtml_tree_node_insert_by_mode(myhtml_tree_t* tree, myhtml_tree_node_t* adjusted_location,
                                                     myhtml_tree_node_t* node, enum myhtml_tree_insertion_mode mode)
{
    if(mode == MyHTML_TREE_INSERTION_MODE_DEFAULT)
        myhtml_tree_node_add_child(tree, adjusted_location, node);
    else if(mode == MyHTML_TREE_INSERTION_MODE_BEFORE)
        myhtml_tree_node_insert_before(tree, adjusted_location, node);
    else
        myhtml_tree_node_insert_after(tree, adjusted_location, node);
}

myhtml_tree_node_t * myhtml_tree_node_insert_by_token(myhtml_tree_t* tree, myhtml_token_node_t* token,
                                                     enum myhtml_namespace my_namespace)
{
    myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
    
    node->tag_idx      = token->tag_ctx_idx;
    node->token        = token;
    node->my_namespace = my_namespace;
    
    enum myhtml_tree_insertion_mode mode;
    myhtml_tree_node_t* adjusted_location = myhtml_tree_appropriate_place_inserting(tree, NULL, &mode);
    myhtml_tree_node_insert_by_mode(tree, adjusted_location, node, mode);
    
    myhtml_tree_open_elements_append(tree, node);
    myhtml_tree_index_append(tree, node);
    
    return node;
}

myhtml_tree_node_t * myhtml_tree_node_insert(myhtml_tree_t* tree, myhtml_tag_id_t tag_idx,
                                            enum myhtml_namespace my_namespace)
{
    myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
    
    node->token        = NULL;
    node->tag_idx      = tag_idx;
    node->my_namespace = my_namespace;
    
    enum myhtml_tree_insertion_mode mode;
    myhtml_tree_node_t* adjusted_location = myhtml_tree_appropriate_place_inserting(tree, NULL, &mode);
    myhtml_tree_node_insert_by_mode(tree, adjusted_location, node, mode);
    
    myhtml_tree_open_elements_append(tree, node);
    myhtml_tree_index_append(tree, node);
    
    return node;
}

myhtml_tree_node_t * myhtml_tree_node_insert_comment(myhtml_tree_t* tree, myhtml_token_node_t* token, myhtml_tree_node_t* parent)
{
    myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
    
    node->token     = token;
    node->tag_idx   = MyHTML_TAG__COMMENT;
    
    enum myhtml_tree_insertion_mode mode = 0;
    if(parent == NULL) {
        parent = myhtml_tree_appropriate_place_inserting(tree, NULL, &mode);
    }
    
    myhtml_tree_node_insert_by_mode(tree, parent, node, mode);
    node->my_namespace = parent->my_namespace;
    
    myhtml_tree_index_append(tree, node);
    
    return node;
}

myhtml_tree_node_t * myhtml_tree_node_insert_doctype(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
    
    node->token        = token;
    node->my_namespace = MyHTML_NAMESPACE_HTML;
    node->tag_idx      = MyHTML_TAG__DOCTYPE;
    
    myhtml_tree_node_add_child(tree, tree->document, node);
    myhtml_tree_index_append(tree, node);
    
    return node;
}

myhtml_tree_node_t * myhtml_tree_node_insert_root(myhtml_tree_t* tree, myhtml_token_node_t* token, enum myhtml_namespace my_namespace)
{
    myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
    
    if(token)
        node->tag_idx = token->tag_ctx_idx;
    else
        node->tag_idx = MyHTML_TAG_HTML;
    
    node->token        = token;
    node->my_namespace = my_namespace;
    
    myhtml_tree_node_add_child(tree, tree->document, node);
    myhtml_tree_open_elements_append(tree, node);
    myhtml_tree_index_append(tree, node);
    
    tree->node_html = node;
    
    return node;
}

myhtml_tree_node_t * myhtml_tree_node_insert_text(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    enum myhtml_tree_insertion_mode mode;
    myhtml_tree_node_t* adjusted_location = myhtml_tree_appropriate_place_inserting(tree, NULL, &mode);
    
    if(adjusted_location == tree->document)
        return NULL;
    
    if(mode == MyHTML_TREE_INSERTION_MODE_AFTER) {
        if(adjusted_location->tag_idx == MyHTML_TAG__TEXT && adjusted_location->token)
        {
            myhtml_token_merged_two_token_string(tree, adjusted_location->token, token, false);
            return adjusted_location;
        }
    }
    else if(mode == MyHTML_TREE_INSERTION_MODE_BEFORE) {
        if(adjusted_location->tag_idx == MyHTML_TAG__TEXT && adjusted_location->token) {
            myhtml_token_merged_two_token_string(tree, token, adjusted_location->token, true);
            return adjusted_location;
        }
    }
    else {
        if(adjusted_location->last_child && adjusted_location->last_child->tag_idx == MyHTML_TAG__TEXT &&
           adjusted_location->last_child->token)
        {
            myhtml_token_merged_two_token_string(tree, adjusted_location->last_child->token, token, false);
            return adjusted_location->last_child;
        }
    }
    
    myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
    
    node->tag_idx      = MyHTML_TAG__TEXT;
    node->token        = token;
    node->my_namespace = adjusted_location->my_namespace;
    
    myhtml_tree_node_insert_by_mode(tree, adjusted_location, node, mode);
    myhtml_tree_index_append(tree, node);
    
    return node;
}

myhtml_tree_node_t * myhtml_tree_node_insert_by_node(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    enum myhtml_tree_insertion_mode mode;
    myhtml_tree_node_t* adjusted_location = myhtml_tree_appropriate_place_inserting(tree, NULL, &mode);
    myhtml_tree_node_insert_by_mode(tree, adjusted_location, node, mode);
    
    myhtml_tree_open_elements_append(tree, node);
    
    return node;
}

myhtml_tree_node_t * myhtml_tree_node_insert_foreign_element(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    enum myhtml_tree_insertion_mode mode;
    myhtml_tree_node_t* adjusted_location = myhtml_tree_appropriate_place_inserting(tree, NULL, &mode);
    
    myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
    
    node->tag_idx      = token->tag_ctx_idx;
    node->token        = token;
    node->my_namespace = adjusted_location->my_namespace;
    
    myhtml_tree_node_insert_by_mode(tree, adjusted_location, node, mode);
    myhtml_tree_open_elements_append(tree, node);
    myhtml_tree_index_append(tree, node);
    
    return node;
}

myhtml_tree_node_t * myhtml_tree_node_insert_html_element(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    enum myhtml_tree_insertion_mode mode;
    myhtml_tree_node_t* adjusted_location = myhtml_tree_appropriate_place_inserting(tree, NULL, &mode);
    
    myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
    
    node->tag_idx      = token->tag_ctx_idx;
    node->token        = token;
    node->my_namespace = MyHTML_NAMESPACE_HTML;
    
    myhtml_tree_node_insert_by_mode(tree, adjusted_location, node, mode);
    myhtml_tree_open_elements_append(tree, node);
    myhtml_tree_index_append(tree, node);
    
    return node;
}

myhtml_tree_node_t * myhtml_tree_element_in_scope(myhtml_tree_t* tree, myhtml_tag_id_t tag_idx, myhtml_namespace_t mynamespace, enum myhtml_tag_categories category)
{
    myhtml_tree_node_t** list = tree->open_elements->list;
    
    const myhtml_tag_context_t *tag_ctx;
    size_t i = tree->open_elements->length;
    
    while(i) {
        i--;
        
        tag_ctx = myhtml_tag_get_by_id(tree->tags, list[i]->tag_idx);
        
        if(list[i]->tag_idx == tag_idx &&
           (mynamespace == MyHTML_NAMESPACE_UNDEF || list[i]->my_namespace == mynamespace))
            return list[i];
        else if(category == MyHTML_TAG_CATEGORIES_SCOPE_SELECT) {
            if((tag_ctx->cats[list[i]->my_namespace] & category) == 0)
                break;
        }
        else if(tag_ctx->cats[list[i]->my_namespace] & category)
            break;
    }
    
    return NULL;
}

bool myhtml_tree_element_in_scope_by_node(myhtml_tree_t* tree, myhtml_tree_node_t* node, enum myhtml_tag_categories category)
{
    myhtml_tree_node_t** list = tree->open_elements->list;
    
    const myhtml_tag_context_t *tag_ctx;
    size_t i = tree->open_elements->length;
    
    while(i) {
        i--;
        
        tag_ctx = myhtml_tag_get_by_id(tree->tags, list[i]->tag_idx);
        
        if(list[i] == node)
            return true;
        else if(category == MyHTML_TAG_CATEGORIES_SCOPE_SELECT) {
            if((tag_ctx->cats[list[i]->my_namespace] & category) == 0)
                break;
        }
        else if(tag_ctx->cats[list[i]->my_namespace] & category)
            break;
    }
    
    return false;
}

// list
myhtml_tree_list_t * myhtml_tree_list_init(void)
{
    myhtml_tree_list_t* list = mymalloc(sizeof(myhtml_tree_list_t));
    
    list->length = 0;
    list->size = 4096;
    list->list = (myhtml_tree_node_t**)mymalloc(sizeof(myhtml_tree_node_t*) * list->size);
    
    return list;
}

void myhtml_tree_list_clean(myhtml_tree_list_t* list)
{
    list->length = 0;
}

myhtml_tree_list_t * myhtml_tree_list_destroy(myhtml_tree_list_t* list, bool destroy_self)
{
    if(list->list)
        free(list->list);
    
    if(destroy_self && list) {
        free(list);
        return NULL;
    }
    
    return list;
}

void myhtml_tree_list_append(myhtml_tree_list_t* list, myhtml_tree_node_t* node)
{
    if(list->length >= list->size) {
        list->size <<= 1;
        
        myhtml_tree_node_t** tmp = (myhtml_tree_node_t**)myrealloc(list->list, sizeof(myhtml_tree_node_t*) * list->size);
        
        if(tmp)
            list->list = tmp;
    }
    
    list->list[list->length] = node;
    list->length++;
}

void myhtml_tree_list_append_after_index(myhtml_tree_list_t* list, myhtml_tree_node_t* node, size_t index)
{
    myhtml_tree_list_insert_by_index(list, node, (index + 1));
}

void myhtml_tree_list_insert_by_index(myhtml_tree_list_t* list, myhtml_tree_node_t* node, size_t index)
{
    if(list->length >= list->size) {
        list->size <<= 1;
        
        myhtml_tree_node_t** tmp = (myhtml_tree_node_t**)myrealloc(list->list, sizeof(myhtml_tree_node_t*) * list->size);
        
        if(tmp)
            list->list = tmp;
    }
    
    myhtml_tree_node_t** node_list = list->list;
    
    memmove(&node_list[(index + 1)], &node_list[index], sizeof(myhtml_tree_node_t**) * (list->length - index));
    
    list->list[index] = node;
    list->length++;
}

myhtml_tree_node_t * myhtml_tree_list_current_node(myhtml_tree_list_t* list)
{
    if(list->length == 0)
        return 0;
    
    return list->list[ list->length - 1 ];
}

// temp stream
struct myhtml_tree_temp_tag_name * myhtml_tree_temp_stream_alloc(myhtml_tree_t* tree, size_t size)
{
    if(tree->temp_stream == NULL) {
        tree->temp_stream = (myhtml_tree_temp_stream_t*)mycalloc(1, sizeof(myhtml_tree_temp_stream_t));
        
        if(tree->temp_stream == NULL)
            return NULL;
        
        tree->temp_stream->length = 0;
        tree->temp_stream->size   = 1024;
        tree->temp_stream->data   = (struct myhtml_tree_temp_tag_name**)mycalloc(tree->temp_stream->size, sizeof(struct myhtml_tree_temp_tag_name));
        
        if(tree->temp_stream->data == NULL)
            return NULL;
    }
    else {
        if(tree->temp_stream->length >= tree->temp_stream->size) {
            size_t new_size = tree->temp_stream->size < 1;
            struct myhtml_tree_temp_tag_name** tmp_data = (struct myhtml_tree_temp_tag_name**)myrealloc(tree->temp_stream->data,
                                                                                                        new_size * sizeof(struct myhtml_tree_temp_tag_name));
            
            if(tmp_data) {
                tree->temp_stream->size = new_size;
                tree->temp_stream->data = tmp_data;
            }
            else
                return NULL;
        }
    }
    
    if(tree->temp_stream->data[ tree->temp_stream->length ] == NULL) {
        tree->temp_stream->data[ tree->temp_stream->length ] = (struct myhtml_tree_temp_tag_name*)mymalloc(sizeof(struct myhtml_tree_temp_tag_name));
        
        if(tree->temp_stream->data[ tree->temp_stream->length ] == NULL)
            return NULL;
    }
    else {
        struct myhtml_tree_temp_tag_name* tmp_data = tree->temp_stream->data[ tree->temp_stream->length ];
        
        if(tmp_data->data && tmp_data->size < size) {
            free(tmp_data->data);
            
            tmp_data->data = NULL;
        }
    }
    
    struct myhtml_tree_temp_tag_name* tmp_data = tree->temp_stream->data[ tree->temp_stream->length ];
    
    tmp_data->length = 0;
    
    if(tmp_data->data == NULL) {
        tmp_data->size = size;
        tmp_data->data = (char *)mymalloc(sizeof(char) * size);
        
        if(tmp_data->data == NULL)
            return NULL;
    }
    
    tree->temp_stream->current = tmp_data;
    tree->temp_stream->length++;
    
    return tmp_data;
}

void myhtml_tree_temp_stream_clean(myhtml_tree_t* tree)
{
    if(tree->temp_stream) {
        tree->temp_stream->length  = 0;
        tree->temp_stream->current = NULL;
        myhtml_encoding_result_clean(&tree->temp_stream->res);
    }
}

myhtml_tree_temp_stream_t * myhtml_tree_temp_stream_free(myhtml_tree_t* tree)
{
    if(tree->temp_stream) {
        if(tree->temp_stream->data)
        {
            for(size_t i = 0; i < tree->temp_stream->size; i++) {
                if(tree->temp_stream->data[i])
                    free(tree->temp_stream->data[i]);
                else
                    break;
            }
            
            free(tree->temp_stream->data);
        }
        
        free(tree->temp_stream);
    }
    
    return NULL;
}

// stack of open elements
myhtml_tree_list_t * myhtml_tree_open_elements_init(myhtml_tree_t* tree)
{
    return myhtml_tree_list_init();
}

void myhtml_tree_open_elements_clean(myhtml_tree_t* tree)
{
    tree->open_elements->length = 0;
}

myhtml_tree_list_t * myhtml_tree_open_elements_destroy(myhtml_tree_t* tree)
{
    return myhtml_tree_list_destroy(tree->open_elements, true);
}

myhtml_tree_node_t * myhtml_tree_current_node(myhtml_tree_t* tree)
{
    if(tree->open_elements->length == 0) {
        MyHTML_DEBUG("Current node; Open elements is 0");
        return 0;
    }
    
    return tree->open_elements->list[ tree->open_elements->length - 1 ];
}

myhtml_tree_node_t * myhtml_tree_adjusted_current_node(myhtml_tree_t* tree)
{
    if(tree->open_elements->length == 1 && tree->fragment)
        return tree->fragment;
    
    return myhtml_tree_current_node(tree);
}

void myhtml_tree_open_elements_append(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    myhtml_tree_list_append(tree->open_elements, node);
}

void myhtml_tree_open_elements_append_after_index(myhtml_tree_t* tree, myhtml_tree_node_t* node, size_t index)
{
    myhtml_tree_list_append_after_index(tree->open_elements, node, index);
}

void myhtml_tree_open_elements_pop(myhtml_tree_t* tree)
{
    if(tree->open_elements->length)
        tree->open_elements->length--;
    
#ifdef DEBUG_MODE
    if(tree->open_elements->length == 0) {
        MyHTML_DEBUG("Pop open elements; Now, Open Elements set 0; Good, if the end of parsing, otherwise is very bad");
    }
#endif
}

void myhtml_tree_open_elements_remove(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    myhtml_tree_node_t** list = tree->open_elements->list;
    
    size_t el_idx = tree->open_elements->length;
    while(el_idx)
    {
        el_idx--;
        
        if(list[el_idx] == node)
        {
            memmove(&list[el_idx], &list[el_idx + 1], sizeof(myhtml_tree_node_t**) * (tree->open_elements->length - el_idx));
            tree->open_elements->length--;
            
            break;
        }
    }
    
#ifdef DEBUG_MODE
    if(tree->open_elements->length == 0) {
        MyHTML_DEBUG("Remove open elements; Now, Open Elements set 0; Good, if the end of parsing, otherwise is very bad");
    }
#endif
}

void myhtml_tree_open_elements_pop_until(myhtml_tree_t* tree, myhtml_tag_id_t tag_idx, myhtml_namespace_t mynamespace, bool is_exclude)
{
    myhtml_tree_node_t** list = tree->open_elements->list;
    
    while(tree->open_elements->length)
    {
        tree->open_elements->length--;
        
        if(list[ tree->open_elements->length ]->tag_idx == tag_idx &&
           // check namespace if set
           (mynamespace == MyHTML_NAMESPACE_UNDEF ||
            list[ tree->open_elements->length ]->my_namespace == mynamespace))
        {
            if(is_exclude)
                tree->open_elements->length++;
            
            break;
        }
    }
    
#ifdef DEBUG_MODE
    if(tree->open_elements->length == 0) {
        MyHTML_DEBUG("Until open elements; Now, Open Elements set 0; Good, if the end of parsing, otherwise is very bad");
    }
#endif
}

void myhtml_tree_open_elements_pop_until_by_node(myhtml_tree_t* tree, myhtml_tree_node_t* node, bool is_exclude)
{
    myhtml_tree_node_t** list = tree->open_elements->list;
    
    while(tree->open_elements->length)
    {
        tree->open_elements->length--;
        
        if(list[ tree->open_elements->length ] == node) {
            if(is_exclude)
                tree->open_elements->length++;
            
            break;
        }
    }
    
#ifdef DEBUG_MODE
    if(tree->open_elements->length == 0) {
        MyHTML_DEBUG("Until by node open elements; Now, Open Elements set 0; Good, if the end of parsing, otherwise is very bad");
    }
#endif
}

void myhtml_tree_open_elements_pop_until_by_index(myhtml_tree_t* tree, size_t idx, bool is_exclude)
{
    while(tree->open_elements->length)
    {
        tree->open_elements->length--;
        
        if(tree->open_elements->length == idx) {
            if(is_exclude)
                tree->open_elements->length++;
            
            break;
        }
    }
    
#ifdef DEBUG_MODE
    if(tree->open_elements->length == 0) {
        MyHTML_DEBUG("Until by index open elements; Now, Open Elements set 0; Good, if the end of parsing, otherwise is very bad");
    }
#endif
}

bool myhtml_tree_open_elements_find_reverse(myhtml_tree_t* tree, myhtml_tree_node_t* idx, size_t* pos)
{
    myhtml_tree_node_t** list = tree->open_elements->list;
    
    size_t i = tree->open_elements->length;
    while(i)
    {
        i--;
        
        if(list[i] == idx) {
            if(pos)
                *pos = i;
            
            return true;
        }
    }
    
    return false;
}

bool myhtml_tree_open_elements_find(myhtml_tree_t* tree, myhtml_tree_node_t* node, size_t* pos)
{
    myhtml_tree_node_t** list = tree->open_elements->list;
    
    for (size_t i = 0; i < tree->open_elements->length; i++)
    {
        if(list[i] == node) {
            if(pos)
                *pos = i;
            
            return true;
        }
    }
    
    return false;
}

myhtml_tree_node_t * myhtml_tree_open_elements_find_by_tag_idx_reverse(myhtml_tree_t* tree, myhtml_tag_id_t tag_idx, myhtml_namespace_t mynamespace, size_t* return_index)
{
    myhtml_tree_node_t** list = tree->open_elements->list;
    
    size_t i = tree->open_elements->length;
    while(i)
    {
        i--;
        
        if(list[i]->tag_idx == tag_idx &&
           (mynamespace == MyHTML_NAMESPACE_UNDEF || list[i]->my_namespace == mynamespace))
        {
            if(return_index)
                *return_index = i;
            
            return list[i];
        }
    }
    
    return NULL;
}

myhtml_tree_node_t * myhtml_tree_open_elements_find_by_tag_idx(myhtml_tree_t* tree, myhtml_tag_id_t tag_idx, myhtml_namespace_t mynamespace, size_t* return_index)
{
    myhtml_tree_node_t** list = tree->open_elements->list;
    
    for (size_t i = 0; i < tree->open_elements->length; i++)
    {
        if(list[i]->tag_idx == tag_idx &&
           (mynamespace == MyHTML_NAMESPACE_UNDEF || list[i]->my_namespace == mynamespace))
        {
            if(return_index)
                *return_index = i;
            
            return list[i];
        }
    }
    
    return NULL;
}

void myhtml_tree_generate_implied_end_tags(myhtml_tree_t* tree, myhtml_tag_id_t exclude_tag_idx, myhtml_namespace_t mynamespace)
{
    if(tree->open_elements->length == 0) {
        MyHTML_DEBUG("Generate implied end tags; Open elements is 0");
        return;
    }
    
    while(tree->open_elements->length > 0)
    {
        myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
        
#ifdef DEBUG_MODE
        if(current_node == NULL) {
            MyHTML_DEBUG_ERROR("Generate implied end tags; Current node is NULL! This is very bad");
        }
#endif
        
        switch (current_node->tag_idx) {
            case MyHTML_TAG_DD:
            case MyHTML_TAG_DT:
            case MyHTML_TAG_LI:
            case MyHTML_TAG_OPTGROUP:
            case MyHTML_TAG_OPTION:
            case MyHTML_TAG_P:
            case MyHTML_TAG_RB:
            case MyHTML_TAG_RP:
            case MyHTML_TAG_RT:
            case MyHTML_TAG_RTC:
                if(exclude_tag_idx == current_node->tag_idx &&
                   (mynamespace == MyHTML_NAMESPACE_UNDEF || current_node->my_namespace == mynamespace))
                    return;
                
                myhtml_tree_open_elements_pop(tree);
                continue;
                
            default:
                return;
        }
    }
}

void myhtml_tree_generate_all_implied_end_tags(myhtml_tree_t* tree, myhtml_tag_id_t exclude_tag_idx, myhtml_namespace_t mynamespace)
{
    if(tree->open_elements->length == 0) {
        MyHTML_DEBUG("Generate all implied end tags; Open elements is 0");
        return;
    }
    
    while(tree->open_elements->length > 0)
    {
        myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
        
#ifdef DEBUG_MODE
        if(current_node == NULL) {
            MyHTML_DEBUG_ERROR("Generate all implied end tags; Current node is NULL! This is very bad");
        }
#endif
        
        switch (current_node->tag_idx) {
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_DD:
            case MyHTML_TAG_DT:
            case MyHTML_TAG_LI:
            case MyHTML_TAG_OPTGROUP:
            case MyHTML_TAG_OPTION:
            case MyHTML_TAG_P:
            case MyHTML_TAG_RB:
            case MyHTML_TAG_RP:
            case MyHTML_TAG_RT:
            case MyHTML_TAG_RTC:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_TH:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
                if(exclude_tag_idx == current_node->tag_idx &&
                   (mynamespace == MyHTML_NAMESPACE_UNDEF || current_node->my_namespace == mynamespace))
                    return;
                
                myhtml_tree_open_elements_pop(tree);
                continue;
                
            default:
                return;
        }
    }
}

void myhtml_tree_reset_insertion_mode_appropriately(myhtml_tree_t* tree)
{
    if(tree->open_elements->length == 0) {
        MyHTML_DEBUG("Reset insertion mode appropriately; Open elements is 0");
        return;
    }
    
    size_t i = tree->open_elements->length;
    
    // step 1
    bool last = false;
    myhtml_tree_node_t** list = tree->open_elements->list;
    
    // step 3
    while(i)
    {
        i--;
        
        // step 2
        myhtml_tree_node_t* node = list[i];
        
#ifdef DEBUG_MODE
        if(node == NULL) {
            MyHTML_DEBUG_ERROR("Reset insertion mode appropriately; node is NULL! This is very bad");
        }
#endif
        
        if(i == 0) {
            last = true;
            
            if(tree->fragment) {
                node = tree->fragment;
            }
        }
        
        if(node->my_namespace != MyHTML_NAMESPACE_HTML) {
            if(last) {
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return;
            }
            
            continue;
        }
        
        // step 4
        if(node->tag_idx == MyHTML_TAG_SELECT)
        {
            // step 4.1
            if(last) {
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_SELECT;
                return;
            }
            
            // step 4.2
            size_t ancestor = i;
            
            while(1)
            {
                // step 4.3
                if(ancestor == 0) {
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_SELECT;
                    return;
                }
                
#ifdef DEBUG_MODE
                if(ancestor == 0) {
                    MyHTML_DEBUG_ERROR("Reset insertion mode appropriately; Ancestor is 0! This is very, very bad");
                }
#endif
                
                // step 4.4
                ancestor--;
                
                // step 4.5
                if(list[ancestor]->tag_idx == MyHTML_TAG_TEMPLATE) {
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_SELECT;
                    return;
                }
                // step 4.6
                else if(list[ancestor]->tag_idx == MyHTML_TAG_TABLE) {
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_SELECT_IN_TABLE;
                    return;
                }
            }
        }
        
        // step 5-15
        switch (node->tag_idx) {
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TH:
                if(last == false) {
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_CELL;
                    return;
                }
                break;
                
            case MyHTML_TAG_TR:
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_ROW;
                return;
                
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE_BODY;
                return;
                
            case MyHTML_TAG_CAPTION:
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_CAPTION;
                return;
                
            case MyHTML_TAG_COLGROUP:
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_COLUMN_GROUP;
                return;
                
            case MyHTML_TAG_TABLE:
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                return;
                
            case MyHTML_TAG_TEMPLATE:
                tree->insert_mode = tree->template_insertion->list[(tree->template_insertion->length - 1)];
                return;
                
            case MyHTML_TAG_HEAD:
                if(last == false) {
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD;
                    return;
                }
                break;
                
            case MyHTML_TAG_BODY:
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return;
                
            case MyHTML_TAG_FRAMESET:
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_FRAMESET;
                return;
                
            case MyHTML_TAG_HTML:
            {
                if(tree->node_head) {
                    tree->insert_mode = MyHTML_INSERTION_MODE_AFTER_HEAD;
                    return;
                }
                
                tree->insert_mode = MyHTML_INSERTION_MODE_BEFORE_HEAD;
                return;
            }
                
            default:
                break;
        }
        
        // step 16
        if(last) {
            tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
            return;
        }
        
        // step 17
    }
    
    tree->insert_mode = MyHTML_INSERTION_MODE_INITIAL;
}

// stack of active formatting elements
myhtml_tree_list_t * myhtml_tree_active_formatting_init(myhtml_tree_t* tree)
{
    return myhtml_tree_list_init();
}

void myhtml_tree_active_formatting_clean(myhtml_tree_t* tree)
{
    tree->active_formatting->length = 0;
}

myhtml_tree_list_t * myhtml_tree_active_formatting_destroy(myhtml_tree_t* tree)
{
    return myhtml_tree_list_destroy(tree->active_formatting, true);
}

bool myhtml_tree_active_formatting_is_marker(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
#ifdef DEBUG_MODE
    if(node == NULL) {
        MyHTML_DEBUG_ERROR("Active formatting is marker; node is NULL!");
    }
#endif
    
    if(tree->myhtml->marker == node)
        return true;
    
    switch (node->tag_idx) {
        case MyHTML_TAG_APPLET:
        case MyHTML_TAG_BUTTON:
        case MyHTML_TAG_OBJECT:
        case MyHTML_TAG_MARQUEE:
        case MyHTML_TAG_TD:
        case MyHTML_TAG_TH:
        case MyHTML_TAG_CAPTION:
            return true;
            
        default:
            break;
    }
    
    return false;
}

void myhtml_tree_active_formatting_append(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    myhtml_tree_list_append( tree->active_formatting, node);
}

void myhtml_tree_active_formatting_pop(myhtml_tree_t* tree)
{
    if(tree->active_formatting->length)
        tree->active_formatting->length--;
    
#ifdef DEBUG_MODE
    if(tree->active_formatting->length == 0) {
        MyHTML_DEBUG("Pop active formatting; length is 0");
    }
#endif
}

void myhtml_tree_active_formatting_remove(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    myhtml_tree_node_t** list = tree->active_formatting->list;
    
    size_t el_idx = tree->active_formatting->length;
    while(el_idx)
    {
        el_idx--;
        
        if(list[el_idx] == node)
        {
            memmove(&list[el_idx], &list[el_idx + 1], sizeof(myhtml_tree_node_t**) * (tree->active_formatting->length - el_idx));
            tree->active_formatting->length--;
            
            break;
        }
    }
    
#ifdef DEBUG_MODE
    if(tree->active_formatting->length == 0) {
        // MyHTML_DEBUG("Remove active formatting; length is 0");
    }
#endif
}

void myhtml_tree_active_formatting_remove_by_index(myhtml_tree_t* tree, size_t idx)
{
    myhtml_tree_node_t** list = tree->active_formatting->list;
    
    memmove(&list[idx], &list[idx + 1], sizeof(myhtml_tree_node_t**) * (tree->active_formatting->length - idx));
    tree->active_formatting->length--;
    
#ifdef DEBUG_MODE
    if(tree->active_formatting->length == 0) {
        MyHTML_DEBUG("Remove active formatting by index; length is 0");
    }
#endif
}

void myhtml_tree_active_formatting_append_with_check(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
//    if(myhtml_tree_active_formatting_is_marker(tree, node)) {
//        myhtml_tree_active_formatting_append(tree, node);
//        return;
//    }
    
    myhtml_tree_node_t** list = tree->active_formatting->list;
    size_t i = tree->active_formatting->length;
    size_t earliest_idx = (i ? (i - 1) : 0);
    size_t count = 0;
    
    while(i)
    {
        i--;
        
#ifdef DEBUG_MODE
        if(list[i] == NULL) {
            MyHTML_DEBUG("Appen active formatting with check; list[%zu] is NULL", i);
        }
#endif
        
        if(myhtml_tree_active_formatting_is_marker(tree, list[i]))
            break;
        
        if(list[i]->token && node->token)
        {
            myhtml_token_node_wait_for_done(list[i]->token);
            myhtml_token_node_wait_for_done(node->token);
            
            if(list[i]->my_namespace == node->my_namespace &&
               list[i]->tag_idx == node->tag_idx &&
               myhtml_token_attr_compare(list[i]->token, node->token))
            {
                count++;
                earliest_idx = i;
            }
        }
    }
    
    if(count >= 3)
        myhtml_tree_active_formatting_remove_by_index(tree, earliest_idx);
    
    myhtml_tree_active_formatting_append(tree, node);
}

myhtml_tree_node_t * myhtml_tree_active_formatting_current_node(myhtml_tree_t* tree)
{
    if(tree->active_formatting->length == 0) {
        MyHTML_DEBUG("Current node active formatting; length is 0");
        return 0;
    }
    
    return tree->active_formatting->list[ tree->active_formatting->length - 1 ];
}

bool myhtml_tree_active_formatting_find(myhtml_tree_t* tree, myhtml_tree_node_t* node, size_t* return_idx)
{
    myhtml_tree_node_t** list = tree->active_formatting->list;
    
    size_t i = tree->active_formatting->length;
    while(i)
    {
        i--;
        
        if(list[i] == node) {
            if(return_idx)
                *return_idx = i;
            
            return true;
        }
    }
    
    return false;
}

void myhtml_tree_active_formatting_up_to_last_marker(myhtml_tree_t* tree)
{
    // Step 1: Let entry be the last (most recently added) entry in the list of active formatting elements.
    myhtml_tree_node_t** list = tree->active_formatting->list;
    
    // Step 2: Remove entry from the list of active formatting elements.
    if(tree->active_formatting->length == 0)
        return;
    
#ifdef DEBUG_MODE
    if(list[ tree->active_formatting->length ] == NULL) {
        MyHTML_DEBUG("Up to last marker active formatting; list[%zu] is NULL", tree->active_formatting->length);
    }
#endif
    
    while(tree->active_formatting->length)
    {
        tree->active_formatting->length--;
        
#ifdef DEBUG_MODE
        if(list[ tree->active_formatting->length ] == NULL) {
            MyHTML_DEBUG("Up to last marker active formatting; list[%zu] is NULL", tree->active_formatting->length);
        }
#endif
        
        if(myhtml_tree_active_formatting_is_marker(tree, list[ tree->active_formatting->length ])) {
            // include marker
            //tree->active_formatting->length++;
            break;
        }
    }
}

myhtml_tree_node_t * myhtml_tree_active_formatting_between_last_marker(myhtml_tree_t* tree, myhtml_tag_id_t tag_idx, size_t* return_idx)
{
    myhtml_tree_node_t** list = tree->active_formatting->list;
    
    size_t i = tree->active_formatting->length;
    while(i)
    {
        i--;
        
#ifdef DEBUG_MODE
        if(list[i] == NULL) {
            MyHTML_DEBUG("Between last marker active formatting; list[%zu] is NULL", i);
        }
#endif
        
        if(myhtml_tree_active_formatting_is_marker(tree, list[i]))
            break;
        else if(list[i]->tag_idx == tag_idx && list[i]->my_namespace == MyHTML_NAMESPACE_HTML) {
            if(return_idx)
                *return_idx = i;
            return list[i];
        }
    }
    
    return NULL;
}

void myhtml_tree_active_formatting_reconstruction(myhtml_tree_t* tree)
{
    myhtml_tree_list_t* af = tree->active_formatting;
    myhtml_tree_node_t** list = af->list;
    
    // step 1
    if(af->length == 0)
        return;
    
    // step 2--3
    size_t af_idx = af->length - 1;
    
    if(myhtml_tree_active_formatting_is_marker(tree, list[af_idx]) ||
       myhtml_tree_open_elements_find(tree, list[af_idx], NULL))
        return;
    
    // step 4--6
    while (af_idx)
    {
        af_idx--;
        
#ifdef DEBUG_MODE
        if(list[af_idx] == NULL) {
            MyHTML_DEBUG("Formatting reconstruction; Step 4--6; list[%zu] is NULL", af_idx);
        }
#endif
        
        if(myhtml_tree_active_formatting_is_marker(tree, list[af_idx]) ||
           myhtml_tree_open_elements_find(tree, list[af_idx], NULL))
        {
            af_idx++; // need if 0
            break;
        }
    }
    
    while (af_idx < af->length)
    {
#ifdef DEBUG_MODE
        if(list[af_idx] == NULL) {
            MyHTML_DEBUG("Formatting reconstruction; Next steps; list[%zu] is NULL", af_idx);
        }
#endif
        
        myhtml_tree_node_t* node = myhtml_tree_node_clone(tree, list[af_idx]);
        myhtml_tree_node_insert_by_node(tree, node);
        
        list[af_idx] = node;
        
        af_idx++;
    }
}

bool myhtml_tree_adoption_agency_algorithm(myhtml_tree_t* tree, myhtml_tag_id_t subject_tag_idx)
{
    if(tree->open_elements->length == 0)
        return false;
    
    size_t oel_curr_index = tree->open_elements->length - 1;
    
    myhtml_tree_node_t**  oel_list     = tree->open_elements->list;
    myhtml_tree_node_t**  afe_list     = tree->active_formatting->list;
    myhtml_tree_node_t*   current_node = oel_list[oel_curr_index];
    
#ifdef DEBUG_MODE
    if(current_node == NULL) {
        MyHTML_DEBUG_ERROR("Adoption agency algorithm; Current node is NULL");
    }
#endif
    
    // step 1
    if(current_node->my_namespace == MyHTML_NAMESPACE_HTML && current_node->tag_idx == subject_tag_idx &&
       myhtml_tree_active_formatting_find(tree, current_node, NULL) == false)
    {
        myhtml_tree_open_elements_pop(tree);
        return false;
    }
    
    // step 2, 3
    int loop = 0;
    
    while (loop < 8)
    {
        // step 4
        loop++;
        
        // step 5
        size_t afe_index = 0;
        myhtml_tree_node_t* formatting_element = NULL;
        {
            myhtml_tree_node_t** list = tree->active_formatting->list;
            
            size_t i = tree->active_formatting->length;
            while(i)
            {
                i--;
                
                if(myhtml_tree_active_formatting_is_marker(tree, list[i]))
                    return false;
                else if(list[i]->tag_idx == subject_tag_idx) {
                    afe_index = i;
                    formatting_element = list[i];
                    break;
                }
            }
        }
        
        //myhtml_tree_node_t* formatting_element = myhtml_tree_active_formatting_between_last_marker(tree, subject_tag_idx, &afe_index);
        
        // If there is no such element, then abort these steps and instead act as described in the
        // ===> "any other end tag" entry above.
        if(formatting_element == NULL) {
            return true;
        }
        
        // step 6
        size_t oel_format_el_idx;
        if(myhtml_tree_open_elements_find(tree, formatting_element, &oel_format_el_idx) == false) {
            myhtml_tree_active_formatting_remove(tree, formatting_element);
            return false;
        }
        
        // step 7
        if(myhtml_tree_element_in_scope_by_node(tree, formatting_element, MyHTML_TAG_CATEGORIES_SCOPE) == false)
            return false;
        
        // step 8
        //if(afe_last != list[i])
        //    fprintf(stderr, "oh");
        
        // step 9
        // Let furthest block be the topmost node in the stack of open elements
        // that is lower in the stack than formatting element, and is an element in the special category. T
        // here might not be one.
        myhtml_tree_node_t* furthest_block = NULL;
        size_t idx_furthest_block = 0;
        for (idx_furthest_block = oel_format_el_idx; idx_furthest_block < tree->open_elements->length; idx_furthest_block++)
        {
            const myhtml_tag_context_t *tag_ctx = myhtml_tag_get_by_id(tree->tags, oel_list[idx_furthest_block]->tag_idx);
            
            if(tag_ctx->cats[oel_list[idx_furthest_block]->my_namespace] & MyHTML_TAG_CATEGORIES_SPECIAL) {
                furthest_block = oel_list[idx_furthest_block];
                break;
            }
        }
        
        // step 10
        // If there is no furthest block, then the UA must first pop all the nodes from the bottom
        // of the stack of open elements, from the current node up to and including formatting element,
        // then remove formatting element from the list of active formatting elements, and finally abort these steps.
        if(furthest_block == NULL)
        {
            while(myhtml_tree_current_node(tree) != formatting_element) {
                myhtml_tree_open_elements_pop(tree);
            }
            
            myhtml_tree_open_elements_pop(tree); // and including formatting element
            myhtml_tree_active_formatting_remove(tree, formatting_element);
            
            return false;
        }
        
#ifdef DEBUG_MODE
        if(oel_format_el_idx == 0) {
            MyHTML_DEBUG_ERROR("Adoption agency algorithm; Step 11; oel_format_el_idx is 0; Bad!");
        }
#endif
        
        // step 11
        myhtml_tree_node_t* common_ancestor = oel_list[oel_format_el_idx - 1];
        
#ifdef DEBUG_MODE
        if(common_ancestor == NULL) {
            MyHTML_DEBUG_ERROR("Adoption agency algorithm; Step 11; common_ancestor is NULL");
        }
#endif
        
        // step 12
        size_t bookmark = afe_index + 1;
        
        // step 13
        myhtml_tree_node_t *node = furthest_block, *last = furthest_block;
        size_t index_oel_node = idx_furthest_block;
        
        // step 13.1
        for(int inner_loop = 0;;)
        {
            // step 13.2
            inner_loop++;
            
            // step 13.3
            size_t node_index;
            if(myhtml_tree_open_elements_find(tree, node, &node_index) == false) {
                node_index = index_oel_node;
            }
            
            if(node_index > 0)
                node_index--;
            else {
                fprintf(stderr, "ERROR: adoption agency algorithm; decrement node_index, node_index is null");
                return false;
            }
            
            index_oel_node = node_index;
            
            node = oel_list[node_index];
            
#ifdef DEBUG_MODE
            if(node == NULL) {
                MyHTML_DEBUG_ERROR("Adoption agency algorithm; Step 13.3; node is NULL");
            }
#endif
            // step 13.4
            if(node == formatting_element)
                break;
            
            // step 13.5
            size_t afe_node_index;
            bool is_exists = myhtml_tree_active_formatting_find(tree, node, &afe_node_index);
            if(inner_loop > 3 && is_exists) {
                myhtml_tree_active_formatting_remove_by_index(tree, afe_node_index);
                
                if(afe_node_index < bookmark)
                    bookmark--;
                
                // If inner loop counter is greater than three and node is in the list of active formatting elements,
                // then remove node from the list of active formatting elements.
                continue; // TODO: realy?
            }
            
            // step 13.6
            if(is_exists == false) {
                myhtml_tree_open_elements_remove(tree, node);
                continue;
            }
            
            // step 13.7
            myhtml_tree_node_t* clone = myhtml_tree_node_clone(tree, node);
            
            clone->my_namespace = MyHTML_NAMESPACE_HTML;
            
            afe_list[afe_node_index] = clone;
            oel_list[node_index] = clone;
            
            node = clone;
            
            // step 13.8
            if(last == furthest_block) {
                bookmark = afe_node_index + 1;
                
#ifdef DEBUG_MODE
                if(bookmark >= tree->active_formatting->length) {
                    MyHTML_DEBUG_ERROR("Adoption agency algorithm; Step 13.8; bookmark >= open_elements length");
                }
#endif
            }
            
            // step 13.9
            if(last->parent)
                myhtml_tree_node_remove(last);
            
            myhtml_tree_node_add_child(tree, node, last);
            
            // step 13.10
            last = node;
        }
        
        if(last->parent)
            myhtml_tree_node_remove(last);
        
        // step 14
        enum myhtml_tree_insertion_mode insert_mode;
        common_ancestor = myhtml_tree_appropriate_place_inserting(tree, common_ancestor, &insert_mode);
        myhtml_tree_node_insert_by_mode(tree, common_ancestor, last, insert_mode);
        
        // step 15
        myhtml_tree_node_t* new_formatting_element = myhtml_tree_node_clone(tree, formatting_element);
        
        new_formatting_element->my_namespace = MyHTML_NAMESPACE_HTML;
        
        // step 16
        myhtml_tree_node_t * furthest_block_child = furthest_block->child;
        
        while (furthest_block_child) {
            myhtml_tree_node_t *next = furthest_block_child->next;
            myhtml_tree_node_remove(furthest_block_child);
            
            myhtml_tree_node_add_child(tree, new_formatting_element, furthest_block_child);
            furthest_block_child = next;
        }
        
        // step 17
        myhtml_tree_node_add_child(tree, furthest_block, new_formatting_element);
        
        // step 18
        if(myhtml_tree_active_formatting_find(tree, formatting_element, &afe_index) == false)
            return false;
        
        if(afe_index < bookmark)
            bookmark--;
        
#ifdef DEBUG_MODE
        if(bookmark >= tree->active_formatting->length) {
            MyHTML_DEBUG_ERROR("Adoption agency algorithm; Before Step 18; bookmark (%zu) >= open_elements length", bookmark);
        }
#endif
        
        myhtml_tree_active_formatting_remove_by_index(tree, afe_index);
        myhtml_tree_list_insert_by_index(tree->active_formatting, new_formatting_element, bookmark);
        
        // step 19
        myhtml_tree_open_elements_remove(tree, formatting_element);
        
        if(myhtml_tree_open_elements_find(tree, furthest_block, &idx_furthest_block)) {
            myhtml_tree_list_insert_by_index(tree->open_elements, new_formatting_element, idx_furthest_block + 1);
        }
        else {
            MyHTML_DEBUG_ERROR("Adoption agency algorithm; Step 19; can't find furthest_block in open elements");
        }
    }
    
    return false;
}

myhtml_tree_node_t * myhtml_tree_appropriate_place_inserting(myhtml_tree_t* tree, myhtml_tree_node_t* override_target,
                                                             enum myhtml_tree_insertion_mode* mode)
{
    *mode = MyHTML_TREE_INSERTION_MODE_DEFAULT;
    
    // step 1
    myhtml_tree_node_t* target = override_target ? override_target : myhtml_tree_current_node(tree);
    
    // step 2
    myhtml_tree_node_t* adjusted_location;
    
    if(tree->foster_parenting) {
#ifdef DEBUG_MODE
        if(target == NULL) {
            MyHTML_DEBUG_ERROR("Appropriate place inserting; Step 2; target is NULL in return value! This IS very bad");
        }
#endif
        if(target->my_namespace != MyHTML_NAMESPACE_HTML)
            return target;
        
        switch (target->tag_idx) {
            case MyHTML_TAG_TABLE:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            {
                size_t idx_template, idx_table;
                
                // step 2.1-2
                myhtml_tree_node_t* last_template = myhtml_tree_open_elements_find_by_tag_idx_reverse(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, &idx_template);
                myhtml_tree_node_t* last_table = myhtml_tree_open_elements_find_by_tag_idx_reverse(tree, MyHTML_TAG_TABLE, MyHTML_NAMESPACE_HTML, &idx_table);
                
                // step 2.3
                if(last_template && (last_table == NULL || idx_template > idx_table))
                {
                    return last_template;
                }
                
                // step 2.4
                else if(last_table == NULL)
                {
                    adjusted_location = tree->open_elements->list[0];
                    break;
                }
                
                // step 2.5
                else if(last_table->parent)
                {
                    //adjusted_location = last_table->parent;
                    
                    if(last_table->prev) {
                        adjusted_location = last_table->prev;
                        *mode = MyHTML_TREE_INSERTION_MODE_AFTER;
                    }
                    else {
                        adjusted_location = last_table;
                        *mode = MyHTML_TREE_INSERTION_MODE_BEFORE;
                    }
                    
                    break;
                }
                
#ifdef DEBUG_MODE
                if(idx_table == 0) {
                    MyHTML_DEBUG_ERROR("Appropriate place inserting; Step 2.5; idx_table is 0");
                }
#endif
                
                // step 2.6-7
                adjusted_location = tree->open_elements->list[idx_table - 1];
                
                break;
            }
                
            default:
                adjusted_location = target;
                break;
        }
    }
    else {
#ifdef DEBUG_MODE
        if(target == NULL) {
            MyHTML_DEBUG_ERROR("Appropriate place inserting; Step 3-5; target is NULL in return value! This IS very bad");
        }
#endif
        
        // step 3-4
        return target;
    }
    
    // step 3-4
    return adjusted_location;
}

myhtml_tree_node_t * myhtml_tree_appropriate_place_inserting_in_tree(myhtml_tree_t* tree, myhtml_tree_node_t* target,
                                                                     enum myhtml_tree_insertion_mode* mode)
{
    *mode = MyHTML_TREE_INSERTION_MODE_BEFORE;
    
    // step 2
    myhtml_tree_node_t* adjusted_location;
    
    if(tree->foster_parenting) {
#ifdef DEBUG_MODE
        if(target == NULL) {
            MyHTML_DEBUG_ERROR("Appropriate place inserting; Step 2; target is NULL in return value! This IS very bad");
        }
#endif
        
        if(target->my_namespace != MyHTML_NAMESPACE_HTML)
            return target;
        
        switch (target->tag_idx) {
            case MyHTML_TAG_TABLE:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            {
                // step 2.1-2
                myhtml_tree_node_t* last_template = myhtml_tree_node_find_parent_by_tag_id(target, MyHTML_TAG_TEMPLATE);
                myhtml_tree_node_t* last_table = myhtml_tree_node_find_parent_by_tag_id(target, MyHTML_TAG_TABLE);
                myhtml_tree_node_t* last_table_in_template = NULL;
                
                if(last_template) {
                    last_table_in_template = myhtml_tree_node_find_parent_by_tag_id(last_template, MyHTML_TAG_TABLE);
                }
                
                // step 2.3
                if(last_template && (last_table == NULL || last_table != last_table_in_template))
                {
                    *mode = MyHTML_TREE_INSERTION_MODE_DEFAULT;
                    adjusted_location = last_template;
                    break;
                }
                
                // step 2.4
                else if(last_table == NULL)
                {
                    adjusted_location = target;
                    break;
                }
                
                // step 2.5
                else if(last_table->parent)
                {
                    //adjusted_location = last_table->parent;
                    
                    if(last_table->prev) {
                        adjusted_location = last_table->prev;
                        *mode = MyHTML_TREE_INSERTION_MODE_AFTER;
                    }
                    else {
                        adjusted_location = last_table;
                    }
                    
                    break;
                }
                
#ifdef DEBUG_MODE
                if(idx_table == 0) {
                    MyHTML_DEBUG_ERROR("Appropriate place inserting; Step 2.5; idx_table is 0");
                }
#endif
                
                // step 2.6-7
                adjusted_location = target;
                
                break;
            }
                
            default:
                *mode = MyHTML_TREE_INSERTION_MODE_DEFAULT;
                adjusted_location = target;
                break;
        }
    }
    else {
#ifdef DEBUG_MODE
        if(target == NULL) {
            MyHTML_DEBUG_ERROR("Appropriate place inserting; Step 3-5; target is NULL in return value! This IS very bad");
        }
#endif
        
        *mode = MyHTML_TREE_INSERTION_MODE_DEFAULT;
        // step 3-4
        return target;
    }
    
    // step 3-4
    return adjusted_location;
}


// stack of template insertion modes
myhtml_tree_insertion_list_t * myhtml_tree_template_insertion_init(myhtml_tree_t* tree)
{
    myhtml_tree_insertion_list_t* list = mymalloc(sizeof(myhtml_tree_insertion_list_t));
    
    list->length = 0;
    list->size = 1024;
    list->list = (enum myhtml_insertion_mode*)mymalloc(sizeof(enum myhtml_insertion_mode) * list->size);
    
    tree->template_insertion = list;
    
    return list;
}

void myhtml_tree_template_insertion_clean(myhtml_tree_t* tree)
{
    tree->template_insertion->length = 0;
}

myhtml_tree_insertion_list_t * myhtml_tree_template_insertion_destroy(myhtml_tree_t* tree)
{
    if(tree->template_insertion->list)
        free(tree->template_insertion->list);
    
    if(tree->template_insertion)
        free(tree->template_insertion);
    
    return NULL;
}

void myhtml_tree_template_insertion_append(myhtml_tree_t* tree, enum myhtml_insertion_mode insert_mode)
{
    myhtml_tree_insertion_list_t* list = tree->template_insertion;
    
    if(list->length >= list->size) {
        list->size <<= 1;
        
        enum myhtml_insertion_mode* tmp = (enum myhtml_insertion_mode*)myrealloc(list->list,
                                                                         sizeof(enum myhtml_insertion_mode) * list->size);
        
        if(tmp)
            list->list = tmp;
    }
    
    list->list[list->length] = insert_mode;
    list->length++;
}

void myhtml_tree_template_insertion_pop(myhtml_tree_t* tree)
{
    if(tree->template_insertion->length)
        tree->template_insertion->length--;

#ifdef DEBUG_MODE
    if(tree->template_insertion->length == 0) {
        MyHTML_DEBUG("Pop template insertion; length is 0");
    }
#endif
}

size_t myhtml_tree_template_insertion_length(myhtml_tree_t* tree)
{
    return tree->template_insertion->length;
}

void myhtml_tree_print_node(myhtml_tree_t* tree, myhtml_tree_node_t* node, FILE* out)
{
    if(node == NULL)
        return;
    
    const myhtml_tag_context_t *ctx = myhtml_tag_get_by_id(tree->tags, node->tag_idx);
    
//    size_t mctree_id = mh_tags_get(node->tag_idx, mctree_id);
//    size_t tag_name_size = mctree_nodes[mctree_id].str_size;
    
    if(node->tag_idx == MyHTML_TAG__TEXT ||
       node->tag_idx == MyHTML_TAG__COMMENT)
    {
        if(node->token)
            fprintf(out, "<%.*s>: %.*s\n", (int)ctx->name_length, ctx->name,
                    (int)node->token->length, &node->token->my_str_tm.data[node->token->begin]);
        else
            fprintf(out, "<%.*s>\n", (int)ctx->name_length, ctx->name);
    }
    else if(node->tag_idx == MyHTML_TAG__DOCTYPE)
    {
        fprintf(out, "<!DOCTYPE");
        
        if(tree->doctype.attr_name) {
            fprintf(out, " %s", tree->doctype.attr_name);
        }
        
        if(tree->doctype.attr_public) {
            fprintf(out, " %s", tree->doctype.attr_public);
        }
        
        if(tree->doctype.attr_system) {
            fprintf(out, " %s", tree->doctype.attr_system);
        }
        
        fprintf(out, ">\n");
    }
    else
    {
#ifdef DEBUG_MODE
        fprintf(out, "<%.*s tagid=\"%zu\" mem=\"%zx\"", (int)tag_name_size, mctree_nodes[mctree_id].str,
                mh_tags_get(node->tag_idx, id), (size_t)node);
#else
        fprintf(out, "<%.*s", (int)ctx->name_length, ctx->name);
        
        if(node->my_namespace != MyHTML_NAMESPACE_HTML) {
            switch (node->my_namespace) {
                case MyHTML_NAMESPACE_SVG:
                    fprintf(out, ":svg");
                    break;
                case MyHTML_NAMESPACE_MATHML:
                    fprintf(out, ":math");
                    break;
                case MyHTML_NAMESPACE_XLINK:
                    fprintf(out, ":xlink");
                    break;
                case MyHTML_NAMESPACE_XML:
                    fprintf(out, ":xml");
                    break;
                case MyHTML_NAMESPACE_XMLNS:
                    fprintf(out, ":xmlns");
                    break;
                default:
                    break;
            }
        }
#endif
        
        if(node->token)
            myhtml_token_print_attr(tree, node->token, out);
        
        fprintf(out, ">\n");
    }
}

void _myhtml_tree_print_node_childs(myhtml_tree_t* tree, myhtml_tree_node_t* node, FILE* out, size_t inc)
{
    if(node == NULL)
        return;
    
    size_t i;
    
    while(node)
    {
        for(i = 0; i < inc; i++)
            fprintf(out, "\t");
        
        myhtml_tree_print_node(tree, node, out);
        _myhtml_tree_print_node_childs(tree, node->child, out, (inc + 1));
        
        node = node->next;
    }
}

void myhtml_tree_print_node_childs(myhtml_tree_t* tree, myhtml_tree_node_t* node, FILE* out, size_t inc)
{
    if(node == NULL)
        return;
    
    _myhtml_tree_print_node_childs(tree, node->child, out, inc);
}

void myhtml_tree_print_by_node(myhtml_tree_t* tree, myhtml_tree_node_t* node, FILE* out, size_t inc)
{
    if(node == NULL)
        return;
    
    myhtml_tree_print_node(tree, node, out);
    myhtml_tree_print_node_childs(tree, node, out, (inc + 1));
}

// token list
myhtml_tree_token_list_t * myhtml_tree_token_list_init(void)
{
    myhtml_tree_token_list_t* list = mymalloc(sizeof(myhtml_tree_token_list_t));
    
    list->length = 0;
    list->size = 4096;
    list->list = (myhtml_token_node_t**)mymalloc(sizeof(myhtml_token_node_t*) * list->size);
    
    return list;
}

void myhtml_tree_token_list_clean(myhtml_tree_token_list_t* list)
{
    list->length = 0;
}

myhtml_tree_token_list_t * myhtml_tree_token_list_destroy(myhtml_tree_token_list_t* list, bool destroy_self)
{
    if(list->list)
        free(list->list);
    
    if(destroy_self && list) {
        free(list);
        return NULL;
    }
    
    return list;
}

void myhtml_tree_token_list_append(myhtml_tree_token_list_t* list, myhtml_token_node_t* token)
{
    if(list->length >= list->size) {
        list->size <<= 1;
        
        myhtml_token_node_t** tmp = (myhtml_token_node_t**)myrealloc(list->list, sizeof(myhtml_token_node_t*) * list->size);
        
        if(tmp)
            list->list = tmp;
    }
    
    list->list[list->length] = token;
    list->length++;
}

void myhtml_tree_token_list_append_after_index(myhtml_tree_token_list_t* list, myhtml_token_node_t* token, size_t index)
{
    if(list->length >= list->size) {
        list->size <<= 1;
        
        myhtml_token_node_t** tmp = (myhtml_token_node_t**)myrealloc(list->list, sizeof(myhtml_token_node_t*) * list->size);
        
        if(tmp)
            list->list = tmp;
    }
    
    myhtml_token_node_t** node_list = list->list;
    size_t el_idx = index;
    
    while(el_idx > list->length) {
        node_list[(el_idx + 1)] = node_list[el_idx];
        el_idx++;
    }
    
    list->list[(index + 1)] = token;
    list->length++;
}

myhtml_token_node_t * myhtml_tree_token_list_current_node(myhtml_tree_token_list_t* list)
{
    if(list->length == 0) {
        MyHTML_DEBUG("Token list current node; length is 0");
        return NULL;
    }
    
    return list->list[ list->length - 1 ];
}

// other
void myhtml_tree_tags_close_p(myhtml_tree_t* tree)
{
    myhtml_tree_generate_implied_end_tags(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML);
    myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, false);
}

myhtml_tree_node_t * myhtml_tree_generic_raw_text_element_parsing_algorithm(myhtml_tree_t* tree, myhtml_token_node_t* token_node)
{
    myhtml_tree_node_t* node = myhtml_tree_node_insert_by_token(tree, token_node, MyHTML_NAMESPACE_HTML);
    
    tree->orig_insert_mode = tree->insert_mode;
    tree->insert_mode      = MyHTML_INSERTION_MODE_TEXT;
    
    return node;
}

void myhtml_tree_clear_stack_back_table_context(myhtml_tree_t* tree)
{
    myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
    
    while(!((current_node->tag_idx   == MyHTML_TAG_TABLE       ||
             current_node->tag_idx      == MyHTML_TAG_TEMPLATE ||
             current_node->tag_idx      == MyHTML_TAG_HTML)    &&
            current_node->my_namespace == MyHTML_NAMESPACE_HTML))
    {
        myhtml_tree_open_elements_pop(tree);
        current_node = myhtml_tree_current_node(tree);
    }
}

void myhtml_tree_clear_stack_back_table_body_context(myhtml_tree_t* tree)
{
    myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
    
    while(!((current_node->tag_idx == MyHTML_TAG_TBODY    ||
             current_node->tag_idx == MyHTML_TAG_TFOOT    ||
             current_node->tag_idx == MyHTML_TAG_THEAD    ||
             current_node->tag_idx == MyHTML_TAG_TEMPLATE ||
             current_node->tag_idx == MyHTML_TAG_HTML)    &&
            current_node->my_namespace == MyHTML_NAMESPACE_HTML))
    {
        myhtml_tree_open_elements_pop(tree);
        current_node = myhtml_tree_current_node(tree);
    }
}

void myhtml_tree_clear_stack_back_table_row_context(myhtml_tree_t* tree)
{
    myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
    
    while(!((current_node->tag_idx == MyHTML_TAG_TR       ||
            current_node->tag_idx == MyHTML_TAG_TEMPLATE  ||
            current_node->tag_idx == MyHTML_TAG_HTML)     &&
           current_node->my_namespace == MyHTML_NAMESPACE_HTML))
    {
        myhtml_tree_open_elements_pop(tree);
        current_node = myhtml_tree_current_node(tree);
    }
}

void myhtml_tree_close_cell(myhtml_tree_t* tree, myhtml_tree_node_t* tr_or_th_node)
{
    // step 1
    myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
    
    // step 2
    myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
    if(!((current_node->tag_idx == MyHTML_TAG_TD ||
       current_node->tag_idx == MyHTML_TAG_TH)  &&
       current_node->my_namespace == MyHTML_NAMESPACE_HTML))
    {
        // parse error
    }
    
    // step 3
    myhtml_tree_open_elements_pop_until(tree, tr_or_th_node->tag_idx, tr_or_th_node->my_namespace, false);
    
    // step 4
    myhtml_tree_active_formatting_up_to_last_marker(tree);
    
    // step 5
    tree->insert_mode = MyHTML_INSERTION_MODE_IN_ROW;
}

bool myhtml_tree_is_mathml_integration_point(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    if(node->my_namespace == MyHTML_NAMESPACE_MATHML &&
       (node->tag_idx == MyHTML_TAG_MI ||
        node->tag_idx == MyHTML_TAG_MO ||
        node->tag_idx == MyHTML_TAG_MN ||
        node->tag_idx == MyHTML_TAG_MS ||
        node->tag_idx == MyHTML_TAG_MTEXT)
       )
        return true;
        
    return false;
}

bool myhtml_tree_is_html_integration_point(myhtml_tree_t* tree, myhtml_tree_node_t* node)
{
    if(node->my_namespace == MyHTML_NAMESPACE_SVG &&
       (node->tag_idx == MyHTML_TAG_FOREIGNOBJECT ||
        node->tag_idx == MyHTML_TAG_DESC ||
        node->tag_idx == MyHTML_TAG_TITLE)
       )
        return true;
    
    if(node->my_namespace == MyHTML_NAMESPACE_MATHML &&
       node->tag_idx == MyHTML_TAG_ANNOTATION_XML && node->token &&
       (node->token->type & MyHTML_TOKEN_TYPE_CLOSE) == 0)
    {
        myhtml_token_node_wait_for_done(node->token);
        
        myhtml_token_attr_t* attr = myhtml_token_attr_match_case(tree->token, node->token,
                                                                 "encoding", 8, "text/html", 9);
        if(attr)
            return true;
        
        attr = myhtml_token_attr_match_case(tree->token, node->token,
                                                            "encoding", 8, "application/xhtml+xml", 21);
        if(attr)
            return true;
    }
    
    return false;
}

// temp tag name
myhtml_status_t myhtml_tree_temp_tag_name_init(myhtml_tree_temp_tag_name_t* temp_tag_name)
{
    temp_tag_name->size   = 1024;
    temp_tag_name->length = 0;
    temp_tag_name->data   = (char *)mymalloc(temp_tag_name->size * sizeof(char));
    
    if(temp_tag_name->data == NULL)
        return MyHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    
    return MyHTML_STATUS_OK;
}

void myhtml_tree_temp_tag_name_clean(myhtml_tree_temp_tag_name_t* temp_tag_name)
{
    temp_tag_name->length = 0;
}

myhtml_tree_temp_tag_name_t * myhtml_tree_temp_tag_name_destroy(myhtml_tree_temp_tag_name_t* temp_tag_name, bool self_destroy)
{
    if(temp_tag_name == NULL)
        return NULL;
    
    if(temp_tag_name->data) {
        free(temp_tag_name->data);
        temp_tag_name->data = NULL;
    }
    
    if(self_destroy) {
        free(temp_tag_name);
        return NULL;
    }
    
    return temp_tag_name;
}

myhtml_status_t myhtml_tree_temp_tag_name_append_one(myhtml_tree_temp_tag_name_t* temp_tag_name, const char name)
{
    if(temp_tag_name->length >= temp_tag_name->size) {
        size_t nsize = temp_tag_name->size << 1;
        char *tmp = (char *)myrealloc(temp_tag_name->data, nsize * sizeof(char));
        
        if(tmp) {
            temp_tag_name->size = nsize;
            temp_tag_name->data = tmp;
        }
        else
            return MyHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    temp_tag_name->data[temp_tag_name->length] = name;
    
    temp_tag_name->length++;
    return MyHTML_STATUS_OK;
}

myhtml_status_t myhtml_tree_temp_tag_name_append(myhtml_tree_temp_tag_name_t* temp_tag_name, const char* name, size_t name_len)
{
    if(name_len == 0)
        return MyHTML_STATUS_OK;
    
    if((temp_tag_name->length + name_len) >= temp_tag_name->size) {
        size_t nsize = (temp_tag_name->size << 1) + name_len;
        char *tmp = (char *)myrealloc(temp_tag_name->data, nsize * sizeof(char));
        
        if(tmp) {
            temp_tag_name->size = nsize;
            temp_tag_name->data = tmp;
        }
        else
            return MyHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        
    }
    
    memcpy(&temp_tag_name->data[temp_tag_name->length], name, name_len);
    
    temp_tag_name->length += name_len;
    return MyHTML_STATUS_OK;
}

void myhtml_tree_wait_for_last_done_token(myhtml_tree_t* tree, myhtml_token_node_t* token_for_wait)
{
#ifndef MyHTML_BUILD_WITHOUT_THREADS
    
    const struct timespec tomeout = {0, 1000};
    while(tree->token_last_done != token_for_wait) {myhtml_thread_nanosleep(&tomeout);}
    
#endif
}

/* special tonek list */
myhtml_status_t myhtml_tree_special_list_init(myhtml_tree_special_token_list_t* special)
{
    special->size   = 1024;
    special->length = 0;
    special->list   = (myhtml_tree_special_token_t *)mymalloc(special->size * sizeof(myhtml_tree_special_token_t));
    
    if(special->list == NULL)
        return MyHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    
    return MyHTML_STATUS_OK;
}

void myhtml_tree_special_list_clean(myhtml_tree_temp_tag_name_t* temp_tag_name)
{
    temp_tag_name->length = 0;
}

myhtml_tree_special_token_list_t * myhtml_tree_special_list_destroy(myhtml_tree_special_token_list_t* special, bool self_destroy)
{
    if(special == NULL)
        return NULL;
    
    if(special->list) {
        free(special->list);
        special->list = NULL;
    }
    
    if(self_destroy) {
        free(special);
        return NULL;
    }
    
    return special;
}

myhtml_status_t myhtml_tree_special_list_append(myhtml_tree_special_token_list_t* special, myhtml_token_node_t *token, myhtml_namespace_t my_namespace)
{
    if(special->length >= special->size) {
        size_t nsize = special->size << 1;
        myhtml_tree_special_token_t *tmp = (myhtml_tree_special_token_t *)myrealloc(special->list, nsize * sizeof(myhtml_tree_special_token_t));
        
        if(tmp) {
            special->size = nsize;
            special->list = tmp;
        }
        else
            return MyHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    special->list[special->length].my_namespace = my_namespace;
    special->list[special->length].token = token;
    
    special->length++;
    return MyHTML_STATUS_OK;
}

size_t myhtml_tree_special_list_length(myhtml_tree_special_token_list_t* special)
{
    return special->length;
}

size_t myhtml_tree_special_list_pop(myhtml_tree_special_token_list_t* special)
{
    if(special->length)
        special->length--;
    
    return special->length;
}


myhtml_tree_special_token_t * myhtml_tree_special_list_get_last(myhtml_tree_special_token_list_t* special)
{
    if(special->length == 0)
        return NULL;
    
    return &special->list[special->length];
}


