/*
 Copyright 2015-2016 Alexander Borisov
 
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

#include "myhtml/tokenizer_end.h"


size_t myhtml_tokenizer_end_state_data(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_size + tree->global_offset), MyHTML_TOKEN_TYPE_DATA);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_tag_open(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    if(qnode->begin < (html_size + tree->global_offset)) {
        if(qnode->begin) {
            qnode->length = (html_offset + tree->global_offset) - qnode->begin;
            myhtml_check_tag_parser(tree, qnode, html, html_offset);
            
            mh_queue_add(tree, html, html_offset, qnode);
        }
        else {
            qnode->token->type ^= (qnode->token->type & MyHTML_TOKEN_TYPE_WHITESPACE);
            myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_size + tree->global_offset), MyHTML_TOKEN_TYPE_DATA);
        }
    }
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_tag_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_size + tree->global_offset), MyHTML_TOKEN_TYPE_DATA);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_end_tag_open(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    if(qnode->begin < (html_size + tree->global_offset))
    {
        qnode->length = (html_offset + tree->global_offset) - qnode->begin;
        qnode->token->type ^= (qnode->token->type & MyHTML_TOKEN_TYPE_WHITESPACE);
        myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_size + tree->global_offset), MyHTML_TOKEN_TYPE_DATA);
    }
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_self_closing_start_tag(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_size + tree->global_offset), MyHTML_TOKEN_TYPE_DATA);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_markup_declaration_open(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    if(qnode->begin > 1) {
        tree->incoming_buf->length = myhtml_tokenizer_state_bogus_comment(tree, qnode, html, qnode->begin, html_size);
        
        if(qnode != tree->current_qnode)
        {
            qnode = tree->current_qnode;
            qnode->length = (html_size + tree->global_offset) - qnode->begin;
            
            if(qnode->length)
            {
                qnode->token->type       ^= (qnode->token->type & MyHTML_TOKEN_TYPE_WHITESPACE);
                qnode->token->tag_ctx_idx = MyHTML_TAG__TEXT;
                qnode->token->type       |= MyHTML_TOKEN_TYPE_DATA;
                
                qnode->length = (html_size + tree->global_offset) - qnode->begin;
                
                mh_queue_add(tree, html, html_offset, qnode);
            }
        }
        else {
            qnode->token->type       ^= (qnode->token->type & MyHTML_TOKEN_TYPE_WHITESPACE);
            qnode->token->tag_ctx_idx = MyHTML_TAG__COMMENT;
            qnode->token->type       |= MyHTML_TOKEN_TYPE_COMMENT;
            
            qnode->length = (html_size + tree->global_offset) - qnode->begin;
            
            mh_queue_add(tree, html, html_offset, qnode);
        }
    }
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_before_attribute_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    mh_queue_add(tree, html, html_offset, qnode);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_attribute_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    tree->attr_current->name_length = (html_offset + tree->global_offset) - tree->attr_current->name_begin;
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_after_attribute_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    mh_queue_add(tree, html, html_offset, qnode);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_before_attribute_value(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    mh_queue_add(tree, html, html_offset, qnode);
    myhtml_token_attr_malloc(tree->token, tree->attr_current, tree->token->mcasync_attr_id);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_attribute_value_double_quoted(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
//    tree->attr_current->value_length = (html_offset + tree->global_offset) - tree->attr_current->value_begin;
//    
//    mh_queue_add(tree, html, html_offset, qnode);
//    myhtml_token_attr_malloc(tree->token, tree->attr_current, tree->token->mcasync_attr_id);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_attribute_value_single_quoted(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
//    tree->attr_current->value_length = (html_offset + tree->global_offset) - tree->attr_current->value_begin;
//    
//    mh_queue_add(tree, html, html_offset, qnode);
//    myhtml_token_attr_malloc(tree->token, tree->attr_current, tree->token->mcasync_attr_id);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_attribute_value_unquoted(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    tree->attr_current->value_length = (html_offset + tree->global_offset) - tree->attr_current->value_begin;
    
    mh_queue_add(tree, html, html_offset, qnode);
    myhtml_token_attr_malloc(tree->token, tree->attr_current, tree->token->mcasync_attr_id);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_comment_start(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    qnode->length = ((html_offset + tree->global_offset) - qnode->begin);
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_comment_start_dash(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    qnode->length = ((html_offset + tree->global_offset) - qnode->begin);
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_comment(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    qnode->length = ((html_offset + tree->global_offset) - qnode->begin);
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_comment_end(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    qnode->length = ((html_offset + tree->global_offset) - qnode->begin);
    
    if(qnode->length > 2) {
        qnode->length -= 2;
        mh_queue_add(tree, html, html_offset, qnode);
    }
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_comment_end_dash(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    qnode->length = ((html_offset + tree->global_offset) - qnode->begin);
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_comment_end_bang(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    qnode->length = ((html_offset + tree->global_offset) - qnode->begin);
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_bogus_comment(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    qnode->length = ((html_offset + tree->global_offset) - qnode->begin);
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_cdata_section(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    qnode->length = ((html_offset + tree->global_offset) - qnode->begin);
    
    if(qnode->length) {
        mh_queue_add(tree, html, html_offset, qnode);
    }
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_rcdata(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    if(qnode->begin < (html_size + tree->global_offset)) {
        qnode->token->type |= MyHTML_TOKEN_TYPE_RCDATA;
        qnode->token->tag_ctx_idx = MyHTML_TAG__TEXT;
        qnode->length = (html_size + tree->global_offset) - qnode->begin;
        
        mh_queue_add(tree, html, 0, qnode);
    }
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_rcdata_less_than_sign(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_RCDATA);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_rcdata_end_tag_open(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_RCDATA);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_rcdata_end_tag_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_RCDATA);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_rawtext(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_RAWTEXT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_rawtext_less_than_sign(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_RAWTEXT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_rawtext_end_tag_open(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_RAWTEXT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_rawtext_end_tag_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_RAWTEXT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_plaintext(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    // all we need inside myhtml_tokenizer_state_plaintext
    return html_offset;
}

size_t myhtml_tokenizer_end_state_doctype(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_RAWTEXT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_before_doctype_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    tree->compat_mode = MyHTML_TREE_COMPAT_MODE_QUIRKS;
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_doctype_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    tree->attr_current->name_length = (html_offset + tree->global_offset) - tree->attr_current->name_begin;
    
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_after_doctype_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    mh_queue_add(tree, html, html_offset, qnode);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_custom_after_doctype_name_a_z(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    mh_queue_add(tree, html, html_offset, qnode);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_before_doctype_public_identifier(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    tree->compat_mode = MyHTML_TREE_COMPAT_MODE_QUIRKS;
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_doctype_public_identifier_double_quoted(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    tree->compat_mode = MyHTML_TREE_COMPAT_MODE_QUIRKS;
    
    if(tree->attr_current->name_begin && html_size) {
        tree->attr_current->name_length = (html_offset + tree->global_offset) - tree->attr_current->name_begin;
    }
    
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_doctype_public_identifier_single_quoted(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_end_state_doctype_public_identifier_double_quoted(tree, qnode, html, (html_offset + tree->global_offset), html_size);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_after_doctype_public_identifier(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    mh_queue_add(tree, html, html_offset, qnode);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_doctype_system_identifier_double_quoted(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    tree->compat_mode = MyHTML_TREE_COMPAT_MODE_QUIRKS;
    
    if(tree->attr_current->name_begin && html_size) {
        tree->attr_current->name_length = (html_offset + tree->global_offset) - tree->attr_current->name_begin;
    }
    
    mh_queue_add(tree, html, html_offset, qnode);
    
    return html_offset;
}

size_t myhtml_tokenizer_end_state_doctype_system_identifier_single_quoted(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_end_state_doctype_system_identifier_double_quoted(tree, qnode, html, (html_offset + tree->global_offset), html_size);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_after_doctype_system_identifier(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    mh_queue_add(tree, html, html_offset, qnode);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_bogus_doctype(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    mh_queue_add(tree, html, html_offset, qnode);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_less_than_sign(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_end_tag_open(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_end_tag_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_escape_start(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_escape_start_dash(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_escaped(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_escaped_dash(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_escaped_dash_dash(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_escaped_less_than_sign(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_escaped_end_tag_open(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_escaped_end_tag_name(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_double_escape_start(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_double_escaped(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_double_escaped_dash(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_double_escaped_dash_dash(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_double_escaped_less_than_sign(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

size_t myhtml_tokenizer_end_state_script_data_double_escape_end(myhtml_tree_t* tree, mythread_queue_node_t* qnode, const char* html, size_t html_offset, size_t html_size)
{
    myhtml_tokenizer_queue_create_text_node_if_need(tree, qnode, html, (html_offset + tree->global_offset), MyHTML_TOKEN_TYPE_SCRIPT);
    return html_offset;
}

