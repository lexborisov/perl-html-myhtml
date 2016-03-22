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

#include "myhtml/rules.h"

void myhtml_insertion_fix_emit_for_text_begin_ws(myhtml_token_node_t* token)
{
    myhtml_token_node_wait_for_done(token);
    myhtml_string_crop_whitespace_from_begin(&token->my_str_tm);
    
    if(token->length != token->my_str_tm.length)
        token->length = token->my_str_tm.length;
}

myhtml_token_node_t * myhtml_insertion_fix_split_for_text_begin_ws(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    myhtml_token_node_wait_for_done(token);
    size_t len = myhtml_string_whitespace_from_begin(&token->my_str_tm);
    
    if(len == 0)
        return NULL;
    
    // create new ws token and insert
    mcobject_async_status_t mcstatus;
    myhtml_token_node_t* new_token = (myhtml_token_node_t*)mcobject_async_malloc(tree->token->nodes_obj, tree->mcasync_token_id, &mcstatus);
    
    if(mcstatus)
        return NULL;
    
    myhtml_token_node_clean(new_token);
    
    myhtml_string_init(tree->mchar, tree->mchar_node_id, &new_token->my_str_tm, (len + 2));
    
    myhtml_string_append(&new_token->my_str_tm, token->my_str_tm.data, len);
    new_token->length = len;
    
    // and cut ws for original
    token->my_str_tm.data = mchar_async_crop_first_chars_without_cache(token->my_str_tm.data, len);
    token->my_str_tm.length -= len;
    token->length = token->my_str_tm.length;
    
    return new_token;
}

bool myhtml_insertion_mode_initial(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    switch (token->tag_ctx_idx)
    {
        case MyHTML_TAG__TEXT:
        {
            if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE) {
                return false;
            }
            
            myhtml_insertion_fix_emit_for_text_begin_ws(token);
            
            // default, other token
            tree->compat_mode = MyHTML_TREE_COMPAT_MODE_QUIRKS;
            tree->insert_mode = MyHTML_INSERTION_MODE_BEFORE_HTML;
            break;
        }
            
        case MyHTML_TAG__COMMENT:
        {
            myhtml_tree_node_insert_comment(tree, token, tree->document);
            return false;
        }
            
        case MyHTML_TAG__DOCTYPE:
        {
            myhtml_token_node_wait_for_done(token);
            
            myhtml_token_release_and_check_doctype_attributes(tree->token, token, &tree->doctype);
            myhtml_tree_node_insert_doctype(tree, token);
            
            // fix for tokenizer
            if(tree->doctype.is_html == false &&
               (tree->doctype.attr_public == NULL ||
               tree->doctype.attr_system == NULL))
            {
                tree->compat_mode = MyHTML_TREE_COMPAT_MODE_QUIRKS;
            }
            
            tree->insert_mode = MyHTML_INSERTION_MODE_BEFORE_HTML;
            return false;
        }
            
        default:
            tree->compat_mode = MyHTML_TREE_COMPAT_MODE_QUIRKS;
            tree->insert_mode = MyHTML_INSERTION_MODE_BEFORE_HTML;
            break;
    }
    
    return true;
}

bool myhtml_insertion_mode_before_html(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_BR:
            case MyHTML_TAG_HTML:
            case MyHTML_TAG_HEAD:
            case MyHTML_TAG_BODY:
            {
                myhtml_tree_node_insert_root(tree, NULL, MyHTML_NAMESPACE_HTML);
                tree->insert_mode = MyHTML_INSERTION_MODE_BEFORE_HEAD;
                return true;
            }
                
            default:
                break;
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__DOCTYPE:
                break;
                
            case MyHTML_TAG__COMMENT:
            {
                myhtml_tree_node_insert_comment(tree, token, tree->document);
                break;
            }
                
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE) {
                    break;
                }
                
                myhtml_insertion_fix_emit_for_text_begin_ws(token);
                
                // default, other token
                myhtml_tree_node_insert_root(tree, NULL, MyHTML_NAMESPACE_HTML);
                tree->insert_mode = MyHTML_INSERTION_MODE_BEFORE_HEAD;
                return true;
            }
                
            case MyHTML_TAG_HTML:
            {
                myhtml_tree_node_insert_root(tree, token, MyHTML_NAMESPACE_HTML);
                tree->insert_mode = MyHTML_INSERTION_MODE_BEFORE_HEAD;
                break;
            }
            
            default:
            {
                myhtml_tree_node_insert_root(tree, NULL, MyHTML_NAMESPACE_HTML);
                tree->insert_mode = MyHTML_INSERTION_MODE_BEFORE_HEAD;
                return true;
            }
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_before_head(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_BR:
            case MyHTML_TAG_HTML:
            case MyHTML_TAG_HEAD:
            case MyHTML_TAG_BODY:
            {
                tree->node_head = myhtml_tree_node_insert(tree, MyHTML_TAG_HEAD, MyHTML_NAMESPACE_HTML);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD;
                return true;
            }
                
            default:
                break;
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE) {
                    break;
                }
                
                myhtml_insertion_fix_emit_for_text_begin_ws(token);
                
                // default, other token
                tree->node_head = myhtml_tree_node_insert(tree, MyHTML_TAG_HEAD, MyHTML_NAMESPACE_HTML);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD;
                return true;
            }
                
            case MyHTML_TAG__COMMENT:
            {
                myhtml_tree_node_insert_comment(tree, token, 0);
                break;
            }
                
            case MyHTML_TAG__DOCTYPE:
                break;
                
            case MyHTML_TAG_HTML:
            {
                return myhtml_insertion_mode_in_body(tree, token);
            }
                
            case MyHTML_TAG_HEAD:
            {
                tree->node_head = myhtml_tree_node_insert_html_element(tree, token);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD;
                break;
            }
                
            default:
            {
                tree->node_head = myhtml_tree_node_insert(tree, MyHTML_TAG_HEAD, MyHTML_NAMESPACE_HTML);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD;
                return true;
            }
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_head(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_HEAD:
            {
                myhtml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHTML_INSERTION_MODE_AFTER_HEAD;
                break;
            }
                
            case MyHTML_TAG_BR:
            case MyHTML_TAG_HTML:
            case MyHTML_TAG_BODY:
            {
                myhtml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHTML_INSERTION_MODE_AFTER_HEAD;
                return true;
            }
                
            case MyHTML_TAG_TEMPLATE:
            {
                if(myhtml_tree_open_elements_find_by_tag_idx_reverse(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, NULL) == NULL)
                    break;
                
                // oh God...
                myhtml_tree_generate_all_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, false);
                myhtml_tree_active_formatting_up_to_last_marker(tree);
                myhtml_tree_template_insertion_pop(tree);
                myhtml_tree_reset_insertion_mode_appropriately(tree);
                
                break;
            }
                
            default:
                break;
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE)
                {
                    myhtml_tree_node_insert_text(tree, token);
                    break;
                }
                
                myhtml_token_node_t* new_token = myhtml_insertion_fix_split_for_text_begin_ws(tree, token);
                if(new_token)
                    myhtml_tree_node_insert_text(tree, new_token);
                
                // default, other token
                myhtml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHTML_INSERTION_MODE_AFTER_HEAD;
                return true;
            }
                
            case MyHTML_TAG__COMMENT:
            {
                myhtml_tree_node_insert_comment(tree, token, 0);
                break;
            }
                
            case MyHTML_TAG__DOCTYPE:
                break;
                
            case MyHTML_TAG_HTML:
            {
                return myhtml_insertion_mode_in_body(tree, token);
            }
                
            case MyHTML_TAG_BASE:
            case MyHTML_TAG_BASEFONT:
            case MyHTML_TAG_BGSOUND:
            case MyHTML_TAG_LINK:
            {
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_open_elements_pop(tree);
                break;
            }
                
            case MyHTML_TAG_META:
            {
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_open_elements_pop(tree);
                
                // if the element has an http-equiv attribute
                break;
            }
                
            case MyHTML_TAG_TITLE:
            {
                myhtml_tree_node_insert_html_element(tree, token);
                
                tree->orig_insert_mode = tree->insert_mode;
                tree->insert_mode = MyHTML_INSERTION_MODE_TEXT;
                tree->state_of_builder = MyHTML_TOKENIZER_STATE_RCDATA;
                
                break;
            }
                
            case MyHTML_TAG_NOSCRIPT:
            {
                if(tree->flags & MyHTML_TREE_FLAGS_SCRIPT) {
                    myhtml_tree_node_insert_html_element(tree, token);
                    
                    tree->orig_insert_mode = tree->insert_mode;
                    tree->insert_mode = MyHTML_INSERTION_MODE_TEXT;
                    tree->state_of_builder = MyHTML_TOKENIZER_STATE_RAWTEXT;
                }
                else {
                    myhtml_tree_node_insert_html_element(tree, token);
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD_NOSCRIPT;
                }
                
                break;
            }
                
            case MyHTML_TAG_STYLE:
            case MyHTML_TAG_NOFRAMES:
            {
                myhtml_tree_node_insert_html_element(tree, token);
                
                tree->orig_insert_mode = tree->insert_mode;
                tree->insert_mode = MyHTML_INSERTION_MODE_TEXT;
                tree->state_of_builder = MyHTML_TOKENIZER_STATE_RAWTEXT;
                
                break;
            }
                
            case MyHTML_TAG_SCRIPT:
            {
                // state 1
                enum myhtml_tree_insertion_mode insert_mode;
                myhtml_tree_node_t* adjusted_location = myhtml_tree_appropriate_place_inserting(tree, NULL, &insert_mode);
                
                // state 2
                myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
                
                node->tag_idx      = MyHTML_TAG_SCRIPT;
                node->token        = token;
                node->my_namespace = MyHTML_NAMESPACE_HTML;
                node->flags        = MyHTML_TREE_NODE_PARSER_INSERTED|MyHTML_TREE_NODE_BLOCKING;
                
                myhtml_tree_node_insert_by_mode(tree, adjusted_location, node, insert_mode);
                myhtml_tree_open_elements_append(tree, node);
                
                tree->orig_insert_mode = tree->insert_mode;
                tree->insert_mode = MyHTML_INSERTION_MODE_TEXT;
                tree->state_of_builder = MyHTML_TOKENIZER_STATE_SCRIPT_DATA;
                
                break;
            }
                
            case MyHTML_TAG_TEMPLATE:
            {
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_active_formatting_append(tree, tree->myhtml->marker); // set marker
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TEMPLATE;
                myhtml_tree_template_insertion_append(tree, MyHTML_INSERTION_MODE_IN_TEMPLATE);
                
                break;
            }
                
            case MyHTML_TAG_HEAD:
                break;
                
            default:
            {
                myhtml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_AFTER_HEAD;
                return true;
            }
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_head_noscript(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_NOSCRIPT:
            {
                myhtml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD;
                break;
            }
                
            case MyHTML_TAG_BR:
            {
                myhtml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD;
                return true;
            }
                
            default:
                break;
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__DOCTYPE:
                break;
                
            case MyHTML_TAG_HTML:
            {
                return myhtml_insertion_mode_in_body(tree, token);
            }
                
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE)
                    return myhtml_insertion_mode_in_head(tree, token);
                
                // default, other token
                myhtml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD;
                return true;
            }
                
            case MyHTML_TAG_BASEFONT:
            case MyHTML_TAG_BGSOUND:
            case MyHTML_TAG_LINK:
            case MyHTML_TAG_META:
            case MyHTML_TAG_NOFRAMES:
            case MyHTML_TAG_STYLE:
            case MyHTML_TAG__COMMENT:
                return myhtml_insertion_mode_in_head(tree, token);
                
            case MyHTML_TAG_HEAD:
            case MyHTML_TAG_NOSCRIPT:
                break;
                
            default:
            {
                myhtml_tree_open_elements_pop(tree);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD;
                return true;
            }
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_after_head(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_BR:
            case MyHTML_TAG_HTML:
            case MyHTML_TAG_BODY:
            {
                tree->node_body = myhtml_tree_node_insert(tree, MyHTML_TAG_BODY, MyHTML_NAMESPACE_HTML);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return true;
            }
                
            case MyHTML_TAG_TEMPLATE:
            {
                return myhtml_insertion_mode_in_head(tree, token);
            }
                
            default:
                break;
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE)
                {
                    myhtml_tree_node_insert_text(tree, token);
                    break;
                }
                
                myhtml_token_node_t* new_token = myhtml_insertion_fix_split_for_text_begin_ws(tree, token);
                if(new_token)
                    myhtml_tree_node_insert_text(tree, new_token);
                
                // default, other token
                tree->node_body = myhtml_tree_node_insert(tree, MyHTML_TAG_BODY, MyHTML_NAMESPACE_HTML);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return true;
            }
                
            case MyHTML_TAG__COMMENT:
                myhtml_tree_node_insert_comment(tree, token, 0);
                break;
                
            case MyHTML_TAG__DOCTYPE:
                break;
                
            case MyHTML_TAG_HTML:
                return myhtml_insertion_mode_in_body(tree, token);
                
            case MyHTML_TAG_BODY:
            {
                tree->node_body = myhtml_tree_node_insert_html_element(tree, token);
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                break;
            }
                
            case MyHTML_TAG_FRAMESET:
                myhtml_tree_node_insert_html_element(tree, token);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_FRAMESET;
                break;
                
            case MyHTML_TAG_BASE:
            case MyHTML_TAG_BASEFONT:
            case MyHTML_TAG_BGSOUND:
            case MyHTML_TAG_LINK:
            case MyHTML_TAG_META:
            case MyHTML_TAG_NOFRAMES:
            case MyHTML_TAG_SCRIPT:
            case MyHTML_TAG_STYLE:
            case MyHTML_TAG_TEMPLATE:
            case MyHTML_TAG_TITLE:
            {
                myhtml_tree_open_elements_append(tree, tree->node_head);
                myhtml_insertion_mode_in_head(tree, token);
                myhtml_tree_open_elements_remove(tree, tree->node_head);
            }
                
            case MyHTML_TAG_HEAD:
            {
                break;
            }
                
            default:
            {
                tree->node_body = myhtml_tree_node_insert(tree, MyHTML_TAG_BODY, MyHTML_NAMESPACE_HTML);
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return true;
            }
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_body_other_end_tag(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    // step 1
    size_t i = tree->open_elements->length;
    while(i) {
        i--;
        
        myhtml_tree_node_t* node = tree->open_elements->list[i];
        
        // step 2
        while (node->tag_idx == token->tag_ctx_idx && node->my_namespace == MyHTML_NAMESPACE_HTML)
        {
            myhtml_tree_generate_implied_end_tags(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML);
            myhtml_tree_open_elements_pop_until_by_node(tree, node, false);
            
            return false;
        }
        
        const myhtml_tag_context_t *tag_ctx = myhtml_tag_get_by_id(tree->tags, node->tag_idx);
        if(tag_ctx->cats[node->my_namespace] & MyHTML_TAG_CATEGORIES_SPECIAL) {
            break;
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_body(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_TEMPLATE:
            {
                return myhtml_insertion_mode_in_head(tree, token);
            }
                
            case MyHTML_TAG_BODY:
            {
                myhtml_tree_node_t* body_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_BODY, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE);
                
                if(body_node == NULL)
                    break;
                
                for (size_t i = 0; i < tree->open_elements->length; i++) {
                    switch (tree->open_elements->list[i]->tag_idx) {
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
                        case MyHTML_TAG_BODY:
                        case MyHTML_TAG_HTML:
                            // set parse error
                            break;
                            
                        default:
                            break;
                    }
                }
                
                tree->insert_mode = MyHTML_INSERTION_MODE_AFTER_BODY;
                break;
            }
                
            case MyHTML_TAG_HTML:
            {
                myhtml_tree_node_t* body_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_BODY, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE);
                
                if(body_node == NULL)
                    break;
                
                for (size_t i = 0; i < tree->open_elements->length; i++) {
                    switch (tree->open_elements->list[i]->tag_idx) {
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
                        case MyHTML_TAG_BODY:
                        case MyHTML_TAG_HTML:
                            // set parse error
                            break;
                            
                        default:
                            break;
                    }
                }
                tree->insert_mode = MyHTML_INSERTION_MODE_AFTER_BODY;
                
                return true;
            }
                
            case MyHTML_TAG_ADDRESS:
            case MyHTML_TAG_ARTICLE:
            case MyHTML_TAG_ASIDE:
            case MyHTML_TAG_BLOCKQUOTE:
            case MyHTML_TAG_BUTTON:
            case MyHTML_TAG_CENTER:
            case MyHTML_TAG_DETAILS:
            case MyHTML_TAG_DIALOG:
            case MyHTML_TAG_DIR:
            case MyHTML_TAG_DIV:
            case MyHTML_TAG_DL:
            case MyHTML_TAG_FIELDSET:
            case MyHTML_TAG_FIGCAPTION:
            case MyHTML_TAG_FIGURE:
            case MyHTML_TAG_FOOTER:
            case MyHTML_TAG_HEADER:
            case MyHTML_TAG_HGROUP:
            case MyHTML_TAG_LISTING:
            case MyHTML_TAG_MAIN:
            case MyHTML_TAG_MENU:
            case MyHTML_TAG_NAV:
            case MyHTML_TAG_OL:
            case MyHTML_TAG_PRE:
            case MyHTML_TAG_SECTION:
            case MyHTML_TAG_SUMMARY:
            case MyHTML_TAG_UL:
            {
                if(myhtml_tree_element_in_scope(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE) == NULL)
                    break;
                
                // step 1
                myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                
                // step 2
                //myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                //if(current_node->my_namespace != MyHTML_NAMESPACE_HTML)
                //    parse error
                
                // step 3
                myhtml_tree_open_elements_pop_until(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, false);
                break;
            }
                
            case MyHTML_TAG_FORM:
            {
                myhtml_tree_node_t* template_node = myhtml_tree_open_elements_find_by_tag_idx(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, NULL);
                
                if(template_node == NULL)
                {
                    // step 1
                    myhtml_tree_node_t* node = tree->node_form;
                    
                    // step 2
                    tree->node_form = NULL;
                    
                    // step 3
                    if(node == NULL || myhtml_tree_element_in_scope_by_node(tree, node, MyHTML_TAG_CATEGORIES_SCOPE) == false) {
                        // parse error
                        break;
                    }
                    
                    // step 4
                    myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                    
                    // step 5
                    //myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                    //if(current_node != node)
                    //    // parse error
                    
                    // step 6
                    myhtml_tree_open_elements_remove(tree, node);
                }
                else {
                    // step 1
                    myhtml_tree_node_t* form_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_FORM, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE);
                    
                    if(form_node)
                        // parse error
                        break;
                    
                    // step 2
                    myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                    
                    // step 3
                    //myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                    //if(current_node != node)
                    //    // parse error
                    
                    // step 4
                    myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_FORM, MyHTML_NAMESPACE_HTML, false);
                }
                
                break;
            }
                
            case MyHTML_TAG_P:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON) == NULL) {
                    // parse error
                    myhtml_tree_node_insert(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML);
                }
                
                myhtml_tree_tags_close_p(tree);
                break;
            }
                
            case MyHTML_TAG_LI:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_LI, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_LIST_ITEM) == NULL) {
                    // parse error
                    break;
                }
                
                // step 1
                myhtml_tree_generate_implied_end_tags(tree, MyHTML_TAG_LI, MyHTML_NAMESPACE_HTML);
                
                // step 2
                //myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                //if(current_node->tag_idx != MyHTML_TAG_LI)
                //    // parse error
                
                // step 3
                myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_LI, MyHTML_NAMESPACE_HTML, false);
                
                break;
            }
               
            case MyHTML_TAG_DT:
            case MyHTML_TAG_DD:
            {
                if(myhtml_tree_element_in_scope(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE) == NULL) {
                    // parse error
                    break;
                }
                
                // step 1
                myhtml_tree_generate_implied_end_tags(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML);
                
                // step 2
                //myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                //if(current_node->tag_idx != token->tag_ctx_idx)
                //    // parse error
                
                // step 3
                myhtml_tree_open_elements_pop_until(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, false);
                
                break;
            }
                
            case MyHTML_TAG_H1:
            case MyHTML_TAG_H2:
            case MyHTML_TAG_H3:
            case MyHTML_TAG_H4:
            case MyHTML_TAG_H5:
            case MyHTML_TAG_H6:
            {
                myhtml_tree_node_t** list = tree->open_elements->list;
                
                myhtml_tree_node_t* node = NULL;
                size_t i = tree->open_elements->length;
                while(i) {
                    i--;
                    
                    const myhtml_tag_context_t *tag_ctx = myhtml_tag_get_by_id(tree->tags, list[i]->tag_idx);
                    
                    if((list[i]->tag_idx == MyHTML_TAG_H1 ||
                       list[i]->tag_idx == MyHTML_TAG_H2  ||
                       list[i]->tag_idx == MyHTML_TAG_H3  ||
                       list[i]->tag_idx == MyHTML_TAG_H4  ||
                       list[i]->tag_idx == MyHTML_TAG_H5  ||
                       list[i]->tag_idx == MyHTML_TAG_H6) &&
                       list[i]->my_namespace == MyHTML_NAMESPACE_HTML) {
                        node = list[i];
                        break;
                    }
                    // TODO: This is wrong!!!
                    else if(tag_ctx->cats[list[i]->my_namespace] & MyHTML_TAG_CATEGORIES_SCOPE)
                        break;
                }
                
                if(node == NULL) {
                    // parse error
                    break;
                }
                
                // step 1
                myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                
                // step 2
                //myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                //if(current_node->tag_idx != token->tag_ctx_idx)
                //    // parse error
                
                // step 3
                while(tree->open_elements->length) {
                    tree->open_elements->length--;
                    
                    if((list[tree->open_elements->length]->tag_idx == MyHTML_TAG_H1 ||
                       list[tree->open_elements->length]->tag_idx == MyHTML_TAG_H2 ||
                       list[tree->open_elements->length]->tag_idx == MyHTML_TAG_H3 ||
                       list[tree->open_elements->length]->tag_idx == MyHTML_TAG_H4 ||
                       list[tree->open_elements->length]->tag_idx == MyHTML_TAG_H5 ||
                       list[tree->open_elements->length]->tag_idx == MyHTML_TAG_H6) &&
                       list[tree->open_elements->length]->my_namespace == MyHTML_NAMESPACE_HTML)
                    {
                        break;
                    }
                }
                
                break;
            }
                
            case MyHTML_TAG_A:
            case MyHTML_TAG_B:
            case MyHTML_TAG_BIG:
            case MyHTML_TAG_CODE:
            case MyHTML_TAG_EM:
            case MyHTML_TAG_FONT:
            case MyHTML_TAG_I:
            case MyHTML_TAG_NOBR:
            case MyHTML_TAG_S:
            case MyHTML_TAG_SMALL:
            case MyHTML_TAG_STRIKE:
            case MyHTML_TAG_STRONG:
            case MyHTML_TAG_TT:
            case MyHTML_TAG_U:
            {
                myhtml_tree_adoption_agency_algorithm(tree, token->tag_ctx_idx);
                    //myhtml_insertion_mode_in_body_other_end_tag(tree, token);
                
                break;
            }
                
            case MyHTML_TAG_APPLET:
            case MyHTML_TAG_MARQUEE:
            case MyHTML_TAG_OBJECT:
            {
                if(myhtml_tree_element_in_scope(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE) == NULL) {
                    // parse error
                    break;
                }
                
                // step 1
                myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                
                // step 2
                //myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                //if(current_node->tag_idx != token->tag_ctx_idx)
                //    // parse error
                
                // step 3
                myhtml_tree_open_elements_pop_until(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, false);
                
                // step 4
                myhtml_tree_active_formatting_up_to_last_marker(tree);
                
                break;
            }
                
            case MyHTML_TAG_BR:
            {
                // TODO: return char element to node
                if(token->attr_first) {
                    token->attr_first = NULL;
                }
                
                if(token->attr_last) {
                    token->attr_last = NULL;
                }
                
                myhtml_tree_active_formatting_reconstruction(tree);
                
                token->type = MyHTML_TOKEN_TYPE_OPEN;
                
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_open_elements_pop(tree);
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
            }
                
            default:
            {
                return myhtml_insertion_mode_in_body_other_end_tag(tree, token);
            }
        }
    }
    // open elements
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_NULL)
                    break;
                
                myhtml_tree_active_formatting_reconstruction(tree);
                myhtml_tree_node_insert_text(tree, token);
                
                if((token->type & MyHTML_TOKEN_TYPE_WHITESPACE) == 0)
                    tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                
                break;
            }
                
            case MyHTML_TAG__COMMENT:
                myhtml_tree_node_insert_comment(tree, token, 0);
                break;
                
            case MyHTML_TAG__DOCTYPE:
                break;
                
            case MyHTML_TAG_HTML:
            {
                if(myhtml_tree_open_elements_find_by_tag_idx(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, NULL))
                    break;
                
                if(tree->open_elements->length > 0) {
                    myhtml_tree_node_t* top_node = tree->open_elements->list[0];
                    
                    if(top_node->token) {
                        myhtml_token_node_wait_for_done(token);
                        myhtml_token_node_wait_for_done(top_node->token);
                        myhtml_token_node_attr_copy_with_check(tree->token, token, top_node->token, tree->mcasync_attr_id);
                    }
                    else {
                        top_node->token = token;
                    }
                }
                
                break;
            }
                
            case MyHTML_TAG_BASE:
            case MyHTML_TAG_BASEFONT:
            case MyHTML_TAG_BGSOUND:
            case MyHTML_TAG_LINK:
            case MyHTML_TAG_META:
            case MyHTML_TAG_NOFRAMES:
            case MyHTML_TAG_SCRIPT:
            case MyHTML_TAG_STYLE:
            case MyHTML_TAG_TEMPLATE:
            case MyHTML_TAG_TITLE:
            {
                return myhtml_insertion_mode_in_head(tree, token);
            }
                
            case MyHTML_TAG_BODY:
            {
                if(tree->open_elements->length > 1)
                {
                    if(!(tree->open_elements->list[1]->tag_idx == MyHTML_TAG_BODY &&
                         tree->open_elements->list[1]->my_namespace == MyHTML_NAMESPACE_HTML) ||
                       myhtml_tree_open_elements_find_by_tag_idx(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, NULL))
                        break;
                }
                else
                    break;
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                
                if(tree->open_elements->length > 1) {
                    myhtml_tree_node_t* top_node = tree->open_elements->list[1];
                    
                    if(top_node->token) {
                        myhtml_token_node_wait_for_done(token);
                        myhtml_token_node_wait_for_done(top_node->token);
                        myhtml_token_node_attr_copy_with_check(tree->token, token, top_node->token, tree->mcasync_attr_id);
                    }
                    else {
                        top_node->token = token;
                    }
                }
                
                break;
            }
                
            case MyHTML_TAG_FRAMESET:
            {
                if(tree->open_elements->length > 1)
                {
                    if(!(tree->open_elements->list[1]->tag_idx == MyHTML_TAG_BODY &&
                         tree->open_elements->list[1]->my_namespace == MyHTML_NAMESPACE_HTML))
                        break;
                }
                else
                    break;
                
                if((tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK) == 0)
                    break;
                
                myhtml_tree_node_t* node = tree->open_elements->list[1];
                
                myhtml_tree_node_remove(node);
                myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_HTML, MyHTML_NAMESPACE_HTML, true);
                
                myhtml_tree_node_insert_html_element(tree, token);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_FRAMESET;
                break;
            }
                
            case MyHTML_TAG__END_OF_FILE:
            {
                if(tree->template_insertion->length)
                    return myhtml_insertion_mode_in_template(tree, token);
                
                myhtml_tree_node_t** list = tree->open_elements->list;
                for(size_t i = 0; i < tree->open_elements->length; i++) {
                    if(list[i]->tag_idx != MyHTML_TAG_DD     && list[i]->tag_idx != MyHTML_TAG_DT       &&
                       list[i]->tag_idx != MyHTML_TAG_LI     && list[i]->tag_idx != MyHTML_TAG_OPTGROUP &&
                       list[i]->tag_idx != MyHTML_TAG_OPTION && list[i]->tag_idx != MyHTML_TAG_P        &&
                       list[i]->tag_idx != MyHTML_TAG_RB     && list[i]->tag_idx != MyHTML_TAG_RP       &&
                       list[i]->tag_idx != MyHTML_TAG_RT     && list[i]->tag_idx != MyHTML_TAG_RTC      &&
                       list[i]->tag_idx != MyHTML_TAG_TBODY  && list[i]->tag_idx != MyHTML_TAG_TD       &&
                       list[i]->tag_idx != MyHTML_TAG_TFOOT  && list[i]->tag_idx != MyHTML_TAG_TH       &&
                       list[i]->tag_idx != MyHTML_TAG_THEAD  && list[i]->tag_idx != MyHTML_TAG_TR       &&
                       list[i]->tag_idx != MyHTML_TAG_BODY   && list[i]->tag_idx != MyHTML_TAG_HTML     &&
                       list[i]->my_namespace != MyHTML_NAMESPACE_HTML)
                    {
                        // parse error
                    }
                }
                
                myhtml_rules_stop_parsing(tree);
                break;
            }
                
            case MyHTML_TAG_ADDRESS:
            case MyHTML_TAG_ARTICLE:
            case MyHTML_TAG_ASIDE:
            case MyHTML_TAG_BLOCKQUOTE:
            case MyHTML_TAG_CENTER:
            case MyHTML_TAG_DETAILS:
            case MyHTML_TAG_DIALOG:
            case MyHTML_TAG_DIR:
            case MyHTML_TAG_DIV:
            case MyHTML_TAG_DL:
            case MyHTML_TAG_FIELDSET:
            case MyHTML_TAG_FIGCAPTION:
            case MyHTML_TAG_FIGURE:
            case MyHTML_TAG_FOOTER:
            case MyHTML_TAG_HEADER:
            case MyHTML_TAG_HGROUP:
            case MyHTML_TAG_MAIN:
            case MyHTML_TAG_MENU:
            case MyHTML_TAG_NAV:
            case MyHTML_TAG_OL:
            case MyHTML_TAG_P:
            case MyHTML_TAG_SECTION:
            case MyHTML_TAG_SUMMARY:
            case MyHTML_TAG_UL:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_node_insert_html_element(tree, token);
                break;
            }
                
            case MyHTML_TAG_H1:
            case MyHTML_TAG_H2:
            case MyHTML_TAG_H3:
            case MyHTML_TAG_H4:
            case MyHTML_TAG_H5:
            case MyHTML_TAG_H6:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                switch (current_node->tag_idx) {
                    case MyHTML_TAG_H1:
                        case MyHTML_TAG_H2:
                        case MyHTML_TAG_H3:
                        case MyHTML_TAG_H4:
                        case MyHTML_TAG_H5:
                        case MyHTML_TAG_H6:
                        
                        if(current_node->my_namespace == MyHTML_NAMESPACE_HTML)
                            myhtml_tree_open_elements_pop(tree);
                        
                        break;
                        
                    default:
                        break;
                }
                
                myhtml_tree_node_insert_html_element(tree, token);
                break;
            }
                
            case MyHTML_TAG_PRE:
            case MyHTML_TAG_LISTING:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_node_insert_html_element(tree, token);
                
                // If the next token is a U+000A LINE FEED (LF) character token, then ignore that token and move on to the next one.
                // (Newlines at the start of pre blocks are ignored as an authoring convenience.)
                // !!! see dispatcher (myhtml_rules_tree_dispatcher) for this
                tree->flags |= MyHTML_TREE_FLAGS_PARSE_FLAG|MyHTML_TREE_FLAGS_PARSE_FLAG_EMIT_NEWLINE;
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                break;
            }
                
            case MyHTML_TAG_FORM:
            {
                myhtml_tree_node_t* is_in_node = myhtml_tree_open_elements_find_by_tag_idx(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, NULL);
                if(tree->node_form && is_in_node == NULL)
                    break;
                
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_node_t* current = myhtml_tree_node_insert_html_element(tree, token);
                
                if(is_in_node == NULL)
                    tree->node_form = current;
                
                break;
            }
                
            case MyHTML_TAG_LI:
            {
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                
                size_t oel_index = tree->open_elements->length;
                
                while (oel_index) {
                    oel_index--;
                    
                    myhtml_tree_node_t* node = tree->open_elements->list[oel_index];
                    const myhtml_tag_context_t *tag_ctx = myhtml_tag_get_by_id(tree->tags, node->tag_idx);
                    
                    if(myhtml_is_html_node(node, MyHTML_TAG_LI)) {
                        myhtml_tree_generate_implied_end_tags(tree, MyHTML_TAG_LI, MyHTML_NAMESPACE_HTML);
                        myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_LI, MyHTML_NAMESPACE_HTML, false);
                        break;
                    }
                    else if(tag_ctx->cats[node->my_namespace] & MyHTML_TAG_CATEGORIES_SPECIAL)
                    {
                        if(!((node->tag_idx == MyHTML_TAG_ADDRESS || node->tag_idx == MyHTML_TAG_DIV ||
                             node->tag_idx == MyHTML_TAG_P) && node->my_namespace == MyHTML_NAMESPACE_HTML))
                                break;
                    }
                }
                
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_node_insert_html_element(tree, token);
                break;
            }
            
            case MyHTML_TAG_DT:
            case MyHTML_TAG_DD:
            {
                // this is copy/past
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                
                size_t oel_index = tree->open_elements->length;
                
                while (oel_index) {
                    oel_index--;
                    
                    myhtml_tree_node_t* node = tree->open_elements->list[oel_index];
                    const myhtml_tag_context_t *tag_ctx = myhtml_tag_get_by_id(tree->tags, node->tag_idx);
                    
                    if(myhtml_is_html_node(node, MyHTML_TAG_DD)) {
                        myhtml_tree_generate_implied_end_tags(tree, MyHTML_TAG_DD, MyHTML_NAMESPACE_HTML);
                        myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_DD, MyHTML_NAMESPACE_HTML, false);
                        break;
                    }
                    else if(myhtml_is_html_node(node, MyHTML_TAG_DT)) {
                        myhtml_tree_generate_implied_end_tags(tree, MyHTML_TAG_DT, MyHTML_NAMESPACE_HTML);
                        myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_DT, MyHTML_NAMESPACE_HTML, false);
                        break;
                    }
                    else if(tag_ctx->cats[node->my_namespace] & MyHTML_TAG_CATEGORIES_SPECIAL)
                    {
                        if(!((node->tag_idx == MyHTML_TAG_ADDRESS || node->tag_idx == MyHTML_TAG_DIV ||
                             node->tag_idx == MyHTML_TAG_P) && node->my_namespace == MyHTML_NAMESPACE_HTML))
                                break;
                    }
                }
                
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_node_insert_html_element(tree, token);
                break;
            }
                
            case MyHTML_TAG_PLAINTEXT:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_node_insert_html_element(tree, token);
                
                tree->state_of_builder = MyHTML_TOKENIZER_STATE_PLAINTEXT;
                break;
            }
                
            case MyHTML_TAG_BUTTON:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_BUTTON, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE)) {
                    myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                    myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_BUTTON, MyHTML_NAMESPACE_HTML, false);
                }
                
                myhtml_tree_active_formatting_reconstruction(tree);
                myhtml_tree_node_insert_html_element(tree, token);
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                break;
            }
               
            case MyHTML_TAG_A:
            {
                myhtml_tree_node_t* node = myhtml_tree_active_formatting_between_last_marker(tree, MyHTML_TAG_A, NULL);
                
                if(node) {
                    myhtml_tree_adoption_agency_algorithm(tree, MyHTML_TAG_A);
                    node = myhtml_tree_active_formatting_between_last_marker(tree, MyHTML_TAG_A, NULL);
                    
                    if(node) {
                        myhtml_tree_open_elements_remove(tree, node);
                        myhtml_tree_active_formatting_remove(tree, node);
                    }
                }
                
                myhtml_tree_active_formatting_reconstruction(tree);
                
                myhtml_tree_node_t* current = myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_active_formatting_append_with_check(tree, current);
                break;
            }
                
            case MyHTML_TAG_B:
            case MyHTML_TAG_BIG:
            case MyHTML_TAG_CODE:
            case MyHTML_TAG_EM:
            case MyHTML_TAG_FONT:
            case MyHTML_TAG_I:
            case MyHTML_TAG_S:
            case MyHTML_TAG_SMALL:
            case MyHTML_TAG_STRIKE:
            case MyHTML_TAG_STRONG:
            case MyHTML_TAG_TT:
            case MyHTML_TAG_U:
            {
                myhtml_tree_active_formatting_reconstruction(tree);
                
                myhtml_tree_node_t* current = myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_active_formatting_append_with_check(tree, current);
                break;
            }

            case MyHTML_TAG_NOBR:
            {
                myhtml_tree_active_formatting_reconstruction(tree);
                
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_NOBR, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE)) {
                    myhtml_tree_adoption_agency_algorithm(tree, MyHTML_TAG_NOBR);
                    myhtml_tree_active_formatting_reconstruction(tree);
                }
                
                myhtml_tree_node_t* current = myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_active_formatting_append_with_check(tree, current);
                break;
            }

            case MyHTML_TAG_APPLET:
            case MyHTML_TAG_MARQUEE:
            case MyHTML_TAG_OBJECT:
            {
                myhtml_tree_active_formatting_reconstruction(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_active_formatting_append(tree, tree->myhtml->marker); // marker
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                break;
            }
                
            case MyHTML_TAG_TABLE:
            {
                if((tree->compat_mode & MyHTML_TREE_COMPAT_MODE_QUIRKS) == 0 &&
                   myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON))
                {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_node_insert_html_element(tree, token);
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                break;
            }
                
            case MyHTML_TAG_AREA:
            case MyHTML_TAG_BR:
            case MyHTML_TAG_EMBED:
            case MyHTML_TAG_IMG:
            case MyHTML_TAG_KEYGEN:
            case MyHTML_TAG_WBR:
            {
                myhtml_tree_active_formatting_reconstruction(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_open_elements_pop(tree);
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                break;
            }
                
            case MyHTML_TAG_INPUT:
            {
                myhtml_tree_active_formatting_reconstruction(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_open_elements_pop(tree);
                
                myhtml_token_node_wait_for_done(token);
                if(myhtml_token_attr_match_case(tree->token, token, "type", 4, "hidden", 6) == NULL) {
                    tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                }
                
                break;
            }
                
            case MyHTML_TAG_MENUITEM:
            case MyHTML_TAG_PARAM:
            case MyHTML_TAG_SOURCE:
            case MyHTML_TAG_TRACK:
            {
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_open_elements_pop(tree);
                break;
            }
                
            case MyHTML_TAG_HR:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_open_elements_pop(tree);
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                break;
            }
                
            case MyHTML_TAG_IMAGE:
            {
                token->tag_ctx_idx = MyHTML_TAG_IMG;
                return true;
            }
                
            case MyHTML_TAG_ISINDEX:
            {
                myhtml_tree_node_t* is_in_node = myhtml_tree_open_elements_find_by_tag_idx(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, NULL);
                if(tree->node_form && is_in_node == NULL)
                    break;
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_node_t* form = myhtml_tree_node_insert(tree, MyHTML_TAG_FORM, MyHTML_NAMESPACE_HTML);
                myhtml_tree_node_t* template = myhtml_tree_open_elements_find_by_tag_idx(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, NULL);
                
                if(template == NULL)
                    tree->node_form = form;
                
                myhtml_token_node_wait_for_done(token);
                myhtml_token_attr_t *attr_action = myhtml_token_attr_remove_by_name(token, "action", 6);
                
                if(attr_action) {
                    myhtml_token_node_malloc(tree->token, form->token, tree->mcasync_token_id);
                    myhtml_token_node_set_done(form->token);
                    
                    myhtml_token_attr_copy(tree->token, attr_action, form->token, tree->mcasync_attr_id);
                }
                
                myhtml_tree_node_insert(tree, MyHTML_TAG_HR, MyHTML_NAMESPACE_HTML);
                myhtml_tree_open_elements_pop(tree);
                
                myhtml_tree_active_formatting_reconstruction(tree);
                
                myhtml_tree_node_insert(tree, MyHTML_TAG_LABEL, MyHTML_NAMESPACE_HTML);
                
                // Insert characters (see below for what they should say).
                // Prompt: If the token has an attribute with the name "prompt",
                // then the first stream of characters must be the same string as given in that attribute,
                // and the second stream of characters must be empty. Otherwise,
                // the two streams of character tokens together should, together with the input element,
                // express the equivalent of "This is a searchable index. Enter search keywords: (input field)" in the user's preferred language.
                
                myhtml_token_attr_t *attr_prompt = myhtml_token_attr_remove_by_name(token, "prompt", 6);
                //myhtml_token_attr_t *attr_name   =
                myhtml_token_attr_remove_by_name(token, "name", 4);
                
                myhtml_tree_node_t* text_node = myhtml_tree_node_insert(tree, MyHTML_TAG__TEXT, MyHTML_NAMESPACE_HTML);
                myhtml_tree_open_elements_pop(tree);
                
                myhtml_token_node_malloc(tree->token, text_node->token, tree->mcasync_token_id);
                myhtml_token_node_set_done(text_node->token);
                
                const char message[] = "This is a searchable index. Enter search keywords: ";
                
                if(attr_prompt && attr_prompt->value_length) {
                    myhtml_token_node_text_append(tree->token, text_node->token, &attr_prompt->entry.data[ attr_prompt->value_begin ], attr_prompt->value_length);
                }
                else {
                    myhtml_token_node_text_append(tree->token, text_node->token, message, strlen(message));
                }
                
                // Insert an HTML element for an "input" start tag token
                // with all the attributes from the "isindex" token except "name", "action", and "prompt",
                // and with an attribute named "name" with the value "isindex".
                // (This creates an input element with the name attribute set to the magic value "isindex".)
                // Immediately pop the current node off the stack of open elements.
                myhtml_tree_node_t* input_node = myhtml_tree_node_insert(tree, MyHTML_TAG_INPUT, MyHTML_NAMESPACE_HTML);
                
                myhtml_token_node_malloc(tree->token, input_node->token, tree->mcasync_token_id);
                myhtml_token_node_set_done(input_node->token);
                
                if(token->attr_first) {
                    myhtml_token_attr_t* attr_isindex = token->attr_first;
                    
                    while (attr_isindex)
                    {
                        myhtml_token_attr_copy(tree->token, attr_isindex, input_node->token, tree->mcasync_attr_id);
                        attr_isindex = attr_isindex->next;
                    }
                }
                myhtml_token_node_attr_append(tree->token, input_node->token, "name", 4, "isindex", 7, tree->mcasync_attr_id);
                myhtml_tree_open_elements_pop(tree);
                
                // Insert more characters (see below for what they should say).
//                text_node = myhtml_tree_node_insert(tree, MyHTML_TAG__TEXT, MyHTML_NAMESPACE_HTML);
//                myhtml_tree_open_elements_pop(tree);
//                
//                myhtml_token_node_malloc(tree->token, text_node->token, tree->mcasync_token_id);
//                myhtml_token_node_set_done(text_node->token);
//                
//                if(attr_prompt && attr_prompt->value_length) {
//                    myhtml_token_node_text_append(tree->token, text_node->token, &attr_prompt->entry.data[ attr_prompt->value_begin ], attr_prompt->value_length);
//                }
//                else {
//                    myhtml_token_node_text_append(tree->token, text_node->token, message, strlen(message));
//                }
                
                // and the end
                myhtml_tree_open_elements_pop(tree);
                
                myhtml_tree_node_insert(tree, MyHTML_TAG_HR, MyHTML_NAMESPACE_HTML);
                myhtml_tree_open_elements_pop(tree);
                
                myhtml_tree_open_elements_pop(tree);
                
                if(template == NULL)
                    tree->node_form = NULL;
                
                break;
            }
                
            case MyHTML_TAG_TEXTAREA:
            {
                myhtml_tree_node_insert_html_element(tree, token);
                
                // If the next token is a U+000A LINE FEED (LF) character token,
                // then ignore that token and move on to the next one.
                // (Newlines at the start of textarea elements are ignored as an authoring convenience.)
                // !!! see dispatcher (myhtml_rules_tree_dispatcher) for this
                tree->flags |= MyHTML_TREE_FLAGS_PARSE_FLAG|MyHTML_TREE_FLAGS_PARSE_FLAG_EMIT_NEWLINE;
                
                tree->orig_insert_mode = tree->insert_mode;
                tree->flags           ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                tree->insert_mode      = MyHTML_INSERTION_MODE_TEXT;
                tree->state_of_builder = MyHTML_TOKENIZER_STATE_RCDATA;
                
                break;
            }

            case MyHTML_TAG_XMP:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_P, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_BUTTON)) {
                    myhtml_tree_tags_close_p(tree);
                }
                
                myhtml_tree_active_formatting_reconstruction(tree);
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                tree->state_of_builder = MyHTML_TOKENIZER_STATE_RAWTEXT;
                
                myhtml_tree_generic_raw_text_element_parsing_algorithm(tree, token);
                break;
            }

            case MyHTML_TAG_IFRAME:
            {
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                tree->state_of_builder = MyHTML_TOKENIZER_STATE_RAWTEXT;
                
                myhtml_tree_generic_raw_text_element_parsing_algorithm(tree, token);
                break;
            }
                
            case MyHTML_TAG_NOEMBED:
            {
                tree->state_of_builder = MyHTML_TOKENIZER_STATE_RAWTEXT;
                myhtml_tree_generic_raw_text_element_parsing_algorithm(tree, token);
                break;
            }
                
            case MyHTML_TAG_NOSCRIPT:
            {
                if(tree->flags & MyHTML_TREE_FLAGS_SCRIPT) {
                    tree->state_of_builder = MyHTML_TOKENIZER_STATE_RAWTEXT;
                    myhtml_tree_generic_raw_text_element_parsing_algorithm(tree, token);
                }
                else {
                    myhtml_tree_active_formatting_reconstruction(tree);
                    myhtml_tree_node_insert_html_element(tree, token);
                }
//                else {
//                    myhtml_tree_node_insert_html_element(tree, token);
//                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_HEAD_NOSCRIPT;
//                }
                
                break;
            }
                
            case MyHTML_TAG_SELECT:
            {
                myhtml_tree_active_formatting_reconstruction(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                
                tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                
                if(tree->insert_mode == MyHTML_INSERTION_MODE_IN_TABLE ||
                   tree->insert_mode == MyHTML_INSERTION_MODE_IN_CAPTION ||
                   tree->insert_mode == MyHTML_INSERTION_MODE_IN_TABLE_BODY ||
                   tree->insert_mode == MyHTML_INSERTION_MODE_IN_ROW ||
                   tree->insert_mode == MyHTML_INSERTION_MODE_IN_CELL)
                {
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_SELECT_IN_TABLE;
                }
                else
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_SELECT;
                
                break;
            }
                
            case MyHTML_TAG_OPTGROUP:
            case MyHTML_TAG_OPTION:
            {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(myhtml_is_html_node(current_node, MyHTML_TAG_OPTION))
                    myhtml_tree_open_elements_pop(tree);
                
                myhtml_tree_active_formatting_reconstruction(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                break;
            }
                
            case MyHTML_TAG_RB:
            case MyHTML_TAG_RTC:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_RUBY, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE)) {
                    myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                }
                
                // myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                // if(current_node->tag_idx != MyHTML_TAG_RUBY) PARSE_ERROR
                
                myhtml_tree_node_insert_html_element(tree, token);
                break;
            }
                
            case MyHTML_TAG_RP:
            case MyHTML_TAG_RT:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_RUBY, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE)) {
                    myhtml_tree_generate_implied_end_tags(tree, MyHTML_TAG_RTC, MyHTML_NAMESPACE_HTML);
                }
                
                // myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                // if(current_node->tag_idx != MyHTML_TAG_RTC ||
                //    current_node->tag_idx != MyHTML_TAG_RUBY) PARSE_ERROR
                
                myhtml_tree_node_insert_html_element(tree, token);
                break;
            }
                
            case MyHTML_TAG_MATH:
            {
                myhtml_tree_active_formatting_reconstruction(tree);
                
                myhtml_token_node_wait_for_done(token);
                
                myhtml_token_adjust_mathml_attributes(token);
                myhtml_token_adjust_foreign_attributes(token);
                
                myhtml_tree_node_t* current_node = myhtml_tree_node_insert_foreign_element(tree, token);
                current_node->my_namespace = MyHTML_NAMESPACE_MATHML;
                
                if(token->type & MyHTML_TOKEN_TYPE_CLOSE_SELF)
                    myhtml_tree_open_elements_pop(tree);
                
                break;
            }
                
            case MyHTML_TAG_SVG:
            {
                myhtml_tree_active_formatting_reconstruction(tree);
                
                myhtml_token_node_wait_for_done(token);
                
                myhtml_token_adjust_svg_attributes(token);
                myhtml_token_adjust_foreign_attributes(token);
                
                myhtml_tree_node_t* current_node = myhtml_tree_node_insert_foreign_element(tree, token);
                current_node->my_namespace = MyHTML_NAMESPACE_SVG;
                
                if(token->type & MyHTML_TOKEN_TYPE_CLOSE_SELF)
                    myhtml_tree_open_elements_pop(tree);
                
                break;
            }
                
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_COL:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_FRAME:
            case MyHTML_TAG_HEAD:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_TH:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            {
                // Ignore this token.
                break;
            }
                
            default:
            {
                myhtml_tree_active_formatting_reconstruction(tree);
                myhtml_tree_node_insert_html_element(tree, token);
                
                break;
            }
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_text(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_SCRIPT:
            {
                // new document.write is not works; set back
                myhtml_tree_open_elements_pop(tree);
                tree->insert_mode = tree->orig_insert_mode;
                break;
            }
                
            default:
            {
                myhtml_tree_open_elements_pop(tree);
                tree->insert_mode = tree->orig_insert_mode;
                break;
            }
        }
    }
    else {
        if(token->tag_ctx_idx == MyHTML_TAG__END_OF_FILE)
        {
            myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
            
            if(current_node->tag_idx == MyHTML_TAG_SCRIPT)
                current_node->flags |= MyHTML_TREE_FLAGS_ALREADY_STARTED;
            
            myhtml_tree_open_elements_pop(tree);
            
            tree->insert_mode = tree->orig_insert_mode;
            return true;
        }
        
        myhtml_tree_node_insert_text(tree, token);
    }
    
    return false;
}

bool myhtml_insertion_mode_in_table(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_TABLE:
            {
                myhtml_tree_node_t* table_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TABLE, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(table_node == NULL)
                     // parse error
                    break;
                
                myhtml_tree_open_elements_pop_until_by_node(tree, table_node, false);
                myhtml_tree_reset_insertion_mode_appropriately(tree);
                
                break;
            }
                
            case MyHTML_TAG_BODY:
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_COL:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_HTML:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_TH:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            {
                // parse error
                break;
            }
                
            case MyHTML_TAG_TEMPLATE:
            {
                return myhtml_insertion_mode_in_head(tree, token);
            }
                
            default: {
                // parse error
                tree->foster_parenting = true;
                myhtml_insertion_mode_in_body(tree, token);
                tree->foster_parenting = false;
                
                break;
            }
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if((current_node->tag_idx == MyHTML_TAG_TABLE  ||
                   current_node->tag_idx  == MyHTML_TAG_TBODY  ||
                   current_node->tag_idx  == MyHTML_TAG_TFOOT  ||
                   current_node->tag_idx  == MyHTML_TAG_THEAD  ||
                   current_node->tag_idx  == MyHTML_TAG_TR)    &&
                   current_node->my_namespace == MyHTML_NAMESPACE_HTML)
                {
                    myhtml_tree_token_list_clean(tree->token_list);
                    
                    tree->orig_insert_mode = tree->insert_mode;
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE_TEXT;
                    
                    return true;
                }
                else {
                    tree->foster_parenting = true;
                    myhtml_insertion_mode_in_body(tree, token);
                    tree->foster_parenting = false;
                    
                    break;
                }
            }
            
            case MyHTML_TAG__COMMENT:
                myhtml_tree_node_insert_comment(tree, token, 0);
                break;
                
            case MyHTML_TAG__DOCTYPE:
                break;
                
            case MyHTML_TAG_CAPTION:
            {
                myhtml_tree_clear_stack_back_table_context(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_active_formatting_append(tree, tree->myhtml->marker);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_CAPTION;
                break;
            }
                
            case MyHTML_TAG_COLGROUP:
            {
                myhtml_tree_clear_stack_back_table_context(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_COLUMN_GROUP;
                break;
            }
                
            case MyHTML_TAG_COL:
            {
                myhtml_tree_clear_stack_back_table_context(tree);
                myhtml_tree_node_insert(tree, MyHTML_TAG_COLGROUP, MyHTML_NAMESPACE_HTML);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_COLUMN_GROUP;
                return true;
            }
                
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
            {
                myhtml_tree_clear_stack_back_table_context(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE_BODY;
                break;
            }
               
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TH:
            case MyHTML_TAG_TR:
            {
                myhtml_tree_clear_stack_back_table_context(tree);
                myhtml_tree_node_insert(tree, MyHTML_TAG_TBODY, MyHTML_NAMESPACE_HTML);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE_BODY;
                return true;
            }
                
            case MyHTML_TAG_TABLE:
            {
                // parse error
                myhtml_tree_node_t* table_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TABLE, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(table_node == NULL)
                    break;
                
                myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_TABLE, MyHTML_NAMESPACE_HTML, false);
                myhtml_tree_reset_insertion_mode_appropriately(tree);
                
                return true;
            }
                
            case MyHTML_TAG_STYLE:
            case MyHTML_TAG_SCRIPT:
            case MyHTML_TAG_TEMPLATE:
            {
                return myhtml_insertion_mode_in_head(tree, token);
            }
                
            case MyHTML_TAG_INPUT:
            {
                myhtml_token_node_wait_for_done(token);
                if(myhtml_token_attr_match_case(tree->token, token, "type", 4, "hidden", 6) == NULL) {
                    tree->foster_parenting = true;
                    myhtml_insertion_mode_in_body(tree, token);
                    tree->foster_parenting = false;
                    
                    break;
                }
                
                // parse error
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_open_elements_pop(tree);
                
                token->type |= MyHTML_TOKEN_TYPE_CLOSE_SELF;
                break;
            }
                
            case MyHTML_TAG_FORM:
            {
                // parse error
                
                myhtml_tree_node_t* template = myhtml_tree_open_elements_find_by_tag_idx(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, NULL);
                if(tree->node_form || template)
                    break;
                
                tree->node_form = myhtml_tree_node_insert_html_element(tree, token);
                
                myhtml_tree_open_elements_pop(tree);
            }
                
            case MyHTML_TAG__END_OF_FILE:
                return myhtml_insertion_mode_in_body(tree, token);
                
            default:
            {
                // parse error
                tree->foster_parenting = true;
                myhtml_insertion_mode_in_body(tree, token);
                tree->foster_parenting = false;
                
                break;
            }
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_table_text(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    // skip NULL, we replaced earlier
    if(token->tag_ctx_idx == MyHTML_TAG__TEXT)
    {
        if(token->type & MyHTML_TOKEN_TYPE_NULL)
            return false;
        
        myhtml_tree_token_list_append(tree->token_list, token);
    }
    else {
        myhtml_tree_token_list_t* token_list = tree->token_list;
        bool is_not_ws = false;
        
        for(size_t i = 0; i < token_list->length; i++) {
            if((token_list->list[i]->type & MyHTML_TOKEN_TYPE_WHITESPACE) == 0) {
                is_not_ws = true;
                break;
            }
        }
        
        if(is_not_ws)
        {
            for(size_t i = 0; i < token_list->length; i++) {
                tree->foster_parenting = true;
                myhtml_insertion_mode_in_body(tree, token_list->list[i]);
                tree->foster_parenting = false;
            }
        }
        else {
            for(size_t i = 0; i < token_list->length; i++) {
                myhtml_tree_node_insert_text(tree, token_list->list[i]);
            }
        }
        
        tree->insert_mode = tree->orig_insert_mode;
        return true;
    }
    
    return false;
}

bool myhtml_insertion_mode_in_caption(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_CAPTION:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_CAPTION, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE) == NULL) {
                    // parse error
                    break;
                }
                
                myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                
                //myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                //if(current_node->tag_idx != MyHTML_TAG_CAPTION) {
                //    // parse error
                //}
                
                myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_CAPTION, MyHTML_NAMESPACE_HTML, false);
                myhtml_tree_active_formatting_up_to_last_marker(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                break;
            }
              
            case MyHTML_TAG_TABLE:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_CAPTION, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE) == NULL) {
                    // parse error
                    break;
                }
                
                myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                
                //myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                //if(current_node->tag_idx != MyHTML_TAG_CAPTION) {
                //    // parse error
                //}
                
                myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_CAPTION, MyHTML_NAMESPACE_HTML, false);
                myhtml_tree_active_formatting_up_to_last_marker(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                return true;
            }
                
            case MyHTML_TAG_BODY:
            case MyHTML_TAG_COL:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_HTML:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_TH:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            {
                break;
            }
                
            default:
                return myhtml_insertion_mode_in_body(tree, token);
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_COL:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_TH:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            {
                if(myhtml_tree_element_in_scope(tree, MyHTML_TAG_CAPTION, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE) == NULL) {
                    // parse error
                    break;
                }
                
                myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                
                //myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                //if(current_node->tag_idx != MyHTML_TAG_CAPTION) {
                //    // parse error
                //}
                
                myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_CAPTION, MyHTML_NAMESPACE_HTML, false);
                myhtml_tree_active_formatting_up_to_last_marker(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                return true;
            }
                
            default:
                return myhtml_insertion_mode_in_body(tree, token);
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_column_group(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_COLGROUP:
            {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(current_node == NULL || !(current_node->tag_idx == MyHTML_TAG_COLGROUP &&
                   current_node->my_namespace == MyHTML_NAMESPACE_HTML))
                {
                    break;
                }
                
                myhtml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                break;
            }
                
            case MyHTML_TAG_COL:
            {
                break;
            }
                
            case MyHTML_TAG_TEMPLATE:
            {
                return myhtml_insertion_mode_in_head(tree, token);
            }
                
            default: {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(current_node && current_node->tag_idx == MyHTML_TAG_COLGROUP &&
                   current_node->my_namespace == MyHTML_NAMESPACE_HTML)
                {
                    myhtml_tree_open_elements_pop(tree);
                    
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                    return true;
                }
                
                break;
            }
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE) {
                    myhtml_tree_node_insert_text(tree, token);
                    break;
                }
                
                myhtml_token_node_t* new_token = myhtml_insertion_fix_split_for_text_begin_ws(tree, token);
                if(new_token)
                    myhtml_tree_node_insert_text(tree, new_token);
                
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(current_node && current_node->tag_idx == MyHTML_TAG_COLGROUP)
                {
                    myhtml_tree_open_elements_pop(tree);
                    
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                    return true;
                }
                
                // parse error
                break;
            }
                
            case MyHTML_TAG__COMMENT:
            {
                myhtml_tree_node_insert_comment(tree, token, 0);
                break;
            }
                
            case MyHTML_TAG__DOCTYPE:
                break;
                
            case MyHTML_TAG_HTML:
            {
                return myhtml_insertion_mode_in_body(tree, token);
            }
                
            case MyHTML_TAG_COL:
            {
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_open_elements_pop(tree);
                break;
            }
                
            case MyHTML_TAG_TEMPLATE:
            {
                return myhtml_insertion_mode_in_head(tree, token);
            }
                
            case MyHTML_TAG__END_OF_FILE:
                return myhtml_insertion_mode_in_body(tree, token);
                
            default:
            {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(current_node && current_node->tag_idx == MyHTML_TAG_COLGROUP &&
                   current_node->my_namespace == MyHTML_NAMESPACE_HTML)
                {
                    myhtml_tree_open_elements_pop(tree);
                    
                    tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                    return true;
                }
                
                // parse error
                break;
            }
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_table_body(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
            {
                myhtml_tree_node_t* node = myhtml_tree_element_in_scope(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(node == NULL)
                    // parse error
                    break;
                
                myhtml_tree_clear_stack_back_table_body_context(tree);
                myhtml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                break;
            }
                
            case MyHTML_TAG_TABLE:
            {
                myhtml_tree_node_t* tbody_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TBODY, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                myhtml_tree_node_t* tfoot_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TFOOT, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                myhtml_tree_node_t* thead_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_THEAD, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(tbody_node == NULL && tfoot_node == NULL && thead_node == NULL)
                    // parse error
                    break;
                
                myhtml_tree_clear_stack_back_table_body_context(tree);
                myhtml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                return true;
            }
               
            case MyHTML_TAG_BODY:
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_COL:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_HTML:
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TH:
            case MyHTML_TAG_TR:
            {
                // parse error
                break;
            }
                
            default:
                return myhtml_insertion_mode_in_table(tree, token);
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG_TR:
            {
                myhtml_tree_clear_stack_back_table_body_context(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_ROW;
                break;
            }
                
            case MyHTML_TAG_TH:
            case MyHTML_TAG_TD:
            {
                // parse error
                myhtml_tree_clear_stack_back_table_body_context(tree);
                
                myhtml_tree_node_insert(tree, MyHTML_TAG_TR, MyHTML_NAMESPACE_HTML);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_ROW;
                return true;
            }
                
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_COL:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
            {
                myhtml_tree_node_t* tbody_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TBODY, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                myhtml_tree_node_t* tfoot_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TFOOT, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                myhtml_tree_node_t* thead_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_THEAD, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(tbody_node == NULL && tfoot_node == NULL && thead_node == NULL)
                    // parse error
                    break;
                
                myhtml_tree_clear_stack_back_table_body_context(tree);
                myhtml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                return true;
            }
                
            default:
                return myhtml_insertion_mode_in_table(tree, token);
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_row(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_TR:
            {
                myhtml_tree_node_t* tr_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TR, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(tr_node == NULL)
                    // parse error
                    break;
                
                myhtml_tree_clear_stack_back_table_row_context(tree);
                
                myhtml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE_BODY;
                break;
            }
                
            case MyHTML_TAG_TABLE:
            {
                myhtml_tree_node_t* tr_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TR, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(tr_node == NULL)
                    // parse error
                    break;
                
                myhtml_tree_clear_stack_back_table_row_context(tree);
                myhtml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE_BODY;
                return true;
            }
                
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
            {
                myhtml_tree_node_t* node = myhtml_tree_element_in_scope(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                if(node == NULL)
                    // parse error
                    break;
                
                myhtml_tree_node_t* tr_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TR, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                if(tr_node == NULL)
                    break;
                
                myhtml_tree_clear_stack_back_table_row_context(tree);
                myhtml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE_BODY;
                return true;
            }
                
            default:
                return myhtml_insertion_mode_in_table(tree, token);
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG_TH:
            case MyHTML_TAG_TD:
            {
                myhtml_tree_clear_stack_back_table_row_context(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_active_formatting_append(tree, tree->myhtml->marker);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_CELL;
                break;
            }
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_COL:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            {
                myhtml_tree_node_t* tr_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TR, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(tr_node == NULL)
                    // parse error
                    break;
                
                myhtml_tree_clear_stack_back_table_row_context(tree);
                myhtml_tree_open_elements_pop(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE_BODY;
                return true;
            }
                
            default:
                return myhtml_insertion_mode_in_table(tree, token);
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_cell(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TH:
            {
                myhtml_tree_node_t* node = myhtml_tree_element_in_scope(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(node == NULL)
                    // parse error
                    break;
                
                myhtml_tree_generate_implied_end_tags(tree, 0, MyHTML_NAMESPACE_UNDEF);
                
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(!(current_node->my_namespace = MyHTML_NAMESPACE_HTML &&
                   current_node->tag_idx == token->tag_ctx_idx))
                {
                    // parse error
                }
                
                myhtml_tree_open_elements_pop_until(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, false);
                
                myhtml_tree_active_formatting_up_to_last_marker(tree);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_ROW;
                break;
            }
                
            case MyHTML_TAG_BODY:
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_COL:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_HTML:
                break;
                
            case MyHTML_TAG_TABLE:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            {
                myhtml_tree_node_t* node = myhtml_tree_element_in_scope(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(node == NULL)
                    // parse error
                    break;
                
                node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TD, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                if(node) {
                    myhtml_tree_close_cell(tree, node);
                }
                else {
                    node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TH, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                    if(node)
                        myhtml_tree_close_cell(tree, node);
                }
                
                return true;
            }
                
            default:
                return myhtml_insertion_mode_in_table(tree, token);
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_COL:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_TH:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            {
                myhtml_tree_node_t* td_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TD, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                myhtml_tree_node_t* th_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_TH, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(td_node == NULL && th_node == NULL)
                    // parse error
                    break;
                
                myhtml_tree_close_cell(tree, (td_node == NULL ? th_node : td_node));
                
                return true;
            }
                
            default:
                return myhtml_insertion_mode_in_body(tree, token);
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_select(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_OPTGROUP:
            {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(current_node->tag_idx == MyHTML_TAG_OPTION &&
                   current_node->my_namespace == MyHTML_NAMESPACE_HTML)
                {
                    if(tree->open_elements->length > 1) {
                        myhtml_tree_node_t *optgrp_node = tree->open_elements->list[ tree->open_elements->length - 2 ];
                        
                        if(optgrp_node->tag_idx == MyHTML_TAG_OPTGROUP &&
                           optgrp_node->my_namespace == MyHTML_NAMESPACE_HTML)
                        {
                            myhtml_tree_open_elements_pop(tree);
                        }
                    }
                    else {
                        MyHTML_DEBUG_ERROR("in select state; open elements length < 2");
                    }
                }
                
                current_node = myhtml_tree_current_node(tree);
                
                if(current_node->tag_idx == MyHTML_TAG_OPTGROUP &&
                   current_node->my_namespace == MyHTML_NAMESPACE_HTML)
                    myhtml_tree_open_elements_pop(tree);
                else
                    // parse error
                    break;
                
                break;
            }
                
            case MyHTML_TAG_OPTION:
            {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(current_node->tag_idx == MyHTML_TAG_OPTION &&
                   current_node->my_namespace == MyHTML_NAMESPACE_HTML)
                    myhtml_tree_open_elements_pop(tree);
                else
                    // parse error
                    break;
                
                break;
            }
                
            case MyHTML_TAG_SELECT:
            {
                // parse error
                myhtml_tree_node_t* select_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_SELECT, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_SELECT);
                
                if(select_node == NULL)
                    // parse error
                    break;
                
                myhtml_tree_open_elements_pop_until_by_node(tree, select_node, false);
                myhtml_tree_reset_insertion_mode_appropriately(tree);
                
                break;
            }
                
            case MyHTML_TAG_TEMPLATE:
                return myhtml_insertion_mode_in_head(tree, token);
                
            default:
                // parse error
                break;
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT: {
                if(token->type & MyHTML_TOKEN_TYPE_NULL)
                    break;
                
                myhtml_tree_node_insert_text(tree, token);
                break;
            }
                
            case MyHTML_TAG__COMMENT:
                myhtml_tree_node_insert_comment(tree, token, NULL);
                break;
                
            case MyHTML_TAG__DOCTYPE:
                break;
                
            case MyHTML_TAG_HTML:
                return myhtml_insertion_mode_in_body(tree, token);
                
            case MyHTML_TAG_OPTION:
            {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(myhtml_is_html_node(current_node, token->tag_ctx_idx))
                    myhtml_tree_open_elements_pop(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                break;
            }
                
            case MyHTML_TAG_OPTGROUP:
            {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(current_node->tag_idx == MyHTML_TAG_OPTION &&
                   current_node->my_namespace == MyHTML_NAMESPACE_HTML)
                    myhtml_tree_open_elements_pop(tree);
                
                current_node = myhtml_tree_current_node(tree);
                
                if(current_node->tag_idx == token->tag_ctx_idx &&
                   current_node->my_namespace == MyHTML_NAMESPACE_HTML)
                    myhtml_tree_open_elements_pop(tree);
                
                myhtml_tree_node_insert_html_element(tree, token);
                break;
            }
                
            case MyHTML_TAG_SELECT:
            {
                // parse error
                myhtml_tree_node_t* select_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_SELECT, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_SELECT);
                
                if(select_node == NULL)
                    break;
                
                myhtml_tree_open_elements_pop_until_by_node(tree, select_node, false);
                myhtml_tree_reset_insertion_mode_appropriately(tree);
                
                break;
            }
                
            case MyHTML_TAG_INPUT:
            case MyHTML_TAG_KEYGEN:
            case MyHTML_TAG_TEXTAREA:
            {
                // parse error
                myhtml_tree_node_t* select_node = myhtml_tree_element_in_scope(tree, MyHTML_TAG_SELECT, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_SELECT);
                
                if(select_node == NULL)
                    break;
                
                myhtml_tree_open_elements_pop_until_by_node(tree, select_node, false);
                myhtml_tree_reset_insertion_mode_appropriately(tree);
                
                return true;
            }
                
            case MyHTML_TAG_SCRIPT:
            case MyHTML_TAG_TEMPLATE:
                return myhtml_insertion_mode_in_head(tree, token);
                
            case MyHTML_TAG__END_OF_FILE:
                return myhtml_insertion_mode_in_body(tree, token);
                
            default:
                // parse error
                break;
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_select_in_table(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_TABLE:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TH:
            {
                // parse error
                myhtml_tree_node_t* some_node = myhtml_tree_element_in_scope(tree, token->tag_ctx_idx, MyHTML_NAMESPACE_HTML, MyHTML_TAG_CATEGORIES_SCOPE_TABLE);
                
                if(some_node == NULL)
                    break;
                
                myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_SELECT, MyHTML_NAMESPACE_HTML, false);
                myhtml_tree_reset_insertion_mode_appropriately(tree);
                
                return true;
            }
                
            default:
                return myhtml_insertion_mode_in_select(tree, token);
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_TABLE:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
            case MyHTML_TAG_TR:
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TH:
            {
                // parse error
                myhtml_tree_open_elements_pop_until(tree, MyHTML_TAG_SELECT, MyHTML_NAMESPACE_HTML, false);
                myhtml_tree_reset_insertion_mode_appropriately(tree);
                
                return true;
            }
            
            default:
                return myhtml_insertion_mode_in_select(tree, token);
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_template(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_TEMPLATE:
                return myhtml_insertion_mode_in_body(tree, token);
                
            default:
                break;
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            case MyHTML_TAG__COMMENT:
            case MyHTML_TAG__DOCTYPE:
                return myhtml_insertion_mode_in_body(tree, token);
                
            case MyHTML_TAG_BASE:
            case MyHTML_TAG_BASEFONT:
            case MyHTML_TAG_BGSOUND:
            case MyHTML_TAG_LINK:
            case MyHTML_TAG_META:
            case MyHTML_TAG_NOFRAMES:
            case MyHTML_TAG_SCRIPT:
            case MyHTML_TAG_STYLE:
            case MyHTML_TAG_TEMPLATE:
            case MyHTML_TAG_TITLE:
                return myhtml_insertion_mode_in_head(tree, token);
                
            case MyHTML_TAG_CAPTION:
            case MyHTML_TAG_COLGROUP:
            case MyHTML_TAG_TBODY:
            case MyHTML_TAG_TFOOT:
            case MyHTML_TAG_THEAD:
                myhtml_tree_template_insertion_pop(tree);
                myhtml_tree_template_insertion_append(tree, MyHTML_INSERTION_MODE_IN_TABLE);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE;
                return true;
                
            case MyHTML_TAG_COL:
                myhtml_tree_template_insertion_pop(tree);
                myhtml_tree_template_insertion_append(tree, MyHTML_INSERTION_MODE_IN_COLUMN_GROUP);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_COLUMN_GROUP;
                return true;
                
            case MyHTML_TAG_TR:
                myhtml_tree_template_insertion_pop(tree);
                myhtml_tree_template_insertion_append(tree, MyHTML_INSERTION_MODE_IN_TABLE_BODY);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_TABLE_BODY;
                return true;
                
            case MyHTML_TAG_TD:
            case MyHTML_TAG_TH:
                myhtml_tree_template_insertion_pop(tree);
                myhtml_tree_template_insertion_append(tree, MyHTML_INSERTION_MODE_IN_ROW);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_ROW;
                return true;
                
            case MyHTML_TAG__END_OF_FILE:
            {
                myhtml_tree_node_t* node = myhtml_tree_open_elements_find_by_tag_idx(tree, MyHTML_TAG_TEMPLATE, MyHTML_NAMESPACE_HTML, NULL);
                
                if(node == NULL) {
                    myhtml_rules_stop_parsing(tree);
                    break;
                }
                
                // parse error
                myhtml_tree_open_elements_pop_until_by_node(tree, node, false);
                myhtml_tree_active_formatting_up_to_last_marker(tree);
                myhtml_tree_template_insertion_pop(tree);
                myhtml_tree_reset_insertion_mode_appropriately(tree);
                
                return true;
            }
                
             default:
                myhtml_tree_template_insertion_pop(tree);
                myhtml_tree_template_insertion_append(tree, MyHTML_INSERTION_MODE_IN_BODY);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return true;
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_after_body(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_HTML:
            {
                if(tree->fragment) {
                    // parse error
                    break;
                }
                
                tree->insert_mode = MyHTML_INSERTION_MODE_AFTER_AFTER_BODY;
                break;
            }
                
            default:
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return true;
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE)
                    return myhtml_insertion_mode_in_body(tree, token);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return true;
            }
                
            case MyHTML_TAG__COMMENT:
            {
                if(tree->open_elements->length == 0) {
                    MyHTML_DEBUG_ERROR("after body state; open_elements length < 1");
                    break;
                }
                
                myhtml_tree_node_t* adjusted_location = tree->open_elements->list[0];
                
                // state 2
                myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
                
                node->tag_idx      = MyHTML_TAG__COMMENT;
                node->token        = token;
                node->my_namespace = adjusted_location->my_namespace;
                
                myhtml_tree_node_add_child(tree, adjusted_location, node);
                
                break;
            }
                
            case MyHTML_TAG__DOCTYPE:
                // parse error
                break;
                
            case MyHTML_TAG_HTML:
                return myhtml_insertion_mode_in_body(tree, token);
                
            case MyHTML_TAG__END_OF_FILE:
                myhtml_rules_stop_parsing(tree);
                break;
                
            default:
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return true;
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_frameset(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_FRAMESET:
            {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(current_node == tree->document->child)
                    // parse error
                    break;
                
                myhtml_tree_open_elements_pop(tree);
                
                current_node = myhtml_tree_current_node(tree);
                
                if(tree->fragment == NULL &&
                   !(current_node->tag_idx == MyHTML_TAG_FRAMESET &&
                     current_node->my_namespace == MyHTML_NAMESPACE_HTML))
                {
                    tree->insert_mode = MyHTML_INSERTION_MODE_AFTER_FRAMESET;
                }
                
                break;
            }
                
            default:
                break;
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE) {
                    myhtml_tree_node_insert_text(tree, token);
                    break;
                }
                
                // parse error
                myhtml_token_node_wait_for_done(token);
                myhtml_string_stay_only_whitespace(&token->my_str_tm);
                
                token->length = token->my_str_tm.length;
                
                // TODO: set to cache chars if length == 0
                if(token->length)
                    myhtml_tree_node_insert_text(tree, token);
                
                break;
            }
                
            case MyHTML_TAG__COMMENT:
            {
                myhtml_tree_node_insert_comment(tree, token, NULL);
                break;
            }
                
            case MyHTML_TAG__DOCTYPE:
                // parse error
                break;
                
            case MyHTML_TAG_HTML:
                return myhtml_insertion_mode_in_body(tree, token);
                
            case MyHTML_TAG_FRAMESET:
                myhtml_tree_node_insert_html_element(tree, token);
                break;
                
            case MyHTML_TAG_FRAME:
                myhtml_tree_node_insert_html_element(tree, token);
                myhtml_tree_open_elements_pop(tree);
                break;
                
            case MyHTML_TAG_NOFRAMES:
                return myhtml_insertion_mode_in_head(tree, token);
                
            case MyHTML_TAG__END_OF_FILE:
            {
                myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
                
                if(current_node == tree->document->child) {
                    // parse error
                }
                
                myhtml_rules_stop_parsing(tree);
                break;
            }
                
            default:
                break;
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_after_frameset(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        switch (token->tag_ctx_idx) {
            case MyHTML_TAG_HTML:
                tree->insert_mode = MyHTML_INSERTION_MODE_AFTER_AFTER_FRAMESET;
                break;
                
            default:
                break;
        }
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE) {
                    myhtml_tree_node_insert_text(tree, token);
                    break;
                }
                
                // parse error
                myhtml_token_node_wait_for_done(token);
                myhtml_string_stay_only_whitespace(&token->my_str_tm);
                
                token->length = token->my_str_tm.length;
                
                // TODO: set to cache chars if length == 0
                if(token->length)
                    myhtml_tree_node_insert_text(tree, token);
                
                break;
            }
                
            case MyHTML_TAG__COMMENT:
            {
                myhtml_tree_node_insert_comment(tree, token, NULL);
                break;
            }
                
            case MyHTML_TAG__DOCTYPE:
                // parse error
                break;
                
            case MyHTML_TAG_HTML:
                return myhtml_insertion_mode_in_body(tree, token);
                
            case MyHTML_TAG_NOFRAMES:
                return myhtml_insertion_mode_in_head(tree, token);
                
            case MyHTML_TAG__END_OF_FILE:
                myhtml_rules_stop_parsing(tree);
                break;
                
            default:
                break;
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_after_after_body(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE)
    {
        tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
        return true;
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__COMMENT:
            {
                myhtml_tree_node_t* adjusted_location = tree->document;
                myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
                
                node->tag_idx      = MyHTML_TAG__COMMENT;
                node->token        = token;
                node->my_namespace = adjusted_location->my_namespace;
                
                myhtml_tree_node_add_child(tree, adjusted_location, node);
                break;
            }
                
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE)
                    return myhtml_insertion_mode_in_body(tree, token);
                
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return true;
            }
                
            case MyHTML_TAG_HTML:
            case MyHTML_TAG__DOCTYPE:
                return myhtml_insertion_mode_in_body(tree, token);
                
            case MyHTML_TAG__END_OF_FILE:
                myhtml_rules_stop_parsing(tree);
                break;
                
            default:
                tree->insert_mode = MyHTML_INSERTION_MODE_IN_BODY;
                return true;
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_after_after_frameset(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE) {
        return false;
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__COMMENT:
            {
                myhtml_tree_node_t* adjusted_location = tree->document;
                myhtml_tree_node_t* node = myhtml_tree_node_create(tree);
                
                node->tag_idx      = MyHTML_TAG__COMMENT;
                node->token        = token;
                node->my_namespace = adjusted_location->my_namespace;
                
                myhtml_tree_node_add_child(tree, adjusted_location, node);
                break;
            }
                
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_WHITESPACE)
                    return myhtml_insertion_mode_in_body(tree, token);
                
                myhtml_token_node_t* new_token = myhtml_insertion_fix_split_for_text_begin_ws(tree, token);
                if(new_token)
                    return myhtml_insertion_mode_in_body(tree, new_token);
                
                // parse error
                break;
            }
                
            case MyHTML_TAG_HTML:
            case MyHTML_TAG__DOCTYPE:
                return myhtml_insertion_mode_in_body(tree, token);
                
            case MyHTML_TAG__END_OF_FILE:
                myhtml_rules_stop_parsing(tree);
                break;
                
            case MyHTML_TAG_NOFRAMES:
                return myhtml_insertion_mode_in_head(tree, token);
                
            default:
                // parse error
                break;
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_foreign_content_end_other(myhtml_tree_t* tree, myhtml_tree_node_t* current_node, myhtml_token_node_t* token)
{
    if(current_node->tag_idx != token->tag_ctx_idx) {
        // parse error
    }
    
    if(tree->open_elements->length)
    {
        myhtml_tree_node_t** list = tree->open_elements->list;
        size_t i = tree->open_elements->length - 1;
        
        while (i)
        {
            current_node = list[i];
            
            if(current_node->tag_idx == token->tag_ctx_idx) {
                myhtml_tree_open_elements_pop_until_by_node(tree, current_node, false);
                return false;
            }
            
            i--;
            
            if(list[i]->my_namespace == MyHTML_NAMESPACE_HTML)
                break;
        }
    }
    
    return tree->myhtml->insertion_func[tree->insert_mode](tree, token);
}

bool myhtml_insertion_mode_in_foreign_content_start_other(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    myhtml_tree_node_t* adjusted_node = myhtml_tree_adjusted_current_node(tree);
    
    myhtml_token_node_wait_for_done(token);
    
    if(adjusted_node->my_namespace == MyHTML_NAMESPACE_MATHML) {
        myhtml_token_adjust_mathml_attributes(token);
    }
    else if(adjusted_node->my_namespace == MyHTML_NAMESPACE_SVG) {
        myhtml_token_adjust_svg_attributes(token);
    }
    
    myhtml_token_adjust_foreign_attributes(token);
    
    myhtml_tree_node_t* node = myhtml_tree_node_insert_foreign_element(tree, token);
    node->my_namespace = adjusted_node->my_namespace;
    
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE_SELF)
    {
        if(token->tag_ctx_idx == MyHTML_TAG_SCRIPT &&
           node->my_namespace == MyHTML_NAMESPACE_SVG)
        {
            return myhtml_insertion_mode_in_foreign_content_end_other(tree, myhtml_tree_current_node(tree), token);
        }
        else {
            myhtml_tree_open_elements_pop(tree);
        }
    }
    
    return false;
}

bool myhtml_insertion_mode_in_foreign_content(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(token->type & MyHTML_TOKEN_TYPE_CLOSE) {
        myhtml_tree_node_t* current_node = myhtml_tree_current_node(tree);
        
        if(token->tag_ctx_idx == MyHTML_TAG_SCRIPT &&
           current_node->tag_idx == MyHTML_TAG_SCRIPT &&
           current_node->my_namespace == MyHTML_NAMESPACE_SVG)
        {
            myhtml_tree_open_elements_pop(tree);
            // TODO: now script is disable, skip this
            return false;
        }
        
        return myhtml_insertion_mode_in_foreign_content_end_other(tree, current_node, token);
    }
    else {
        switch (token->tag_ctx_idx)
        {
            case MyHTML_TAG__TEXT:
            {
                if(token->type & MyHTML_TOKEN_TYPE_NULL) {
                    myhtml_token_node_wait_for_done(token);
                    myhtml_token_set_replacement_character_for_null_token(token);
                }
                
                myhtml_tree_node_insert_text(tree, token);
                
                if((token->type & MyHTML_TOKEN_TYPE_WHITESPACE) == 0)
                    tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_FRAMESET_OK);
                
                break;
            }
                
            case MyHTML_TAG__COMMENT:
                myhtml_tree_node_insert_comment(tree, token, NULL);
                break;
                
            case MyHTML_TAG__DOCTYPE:
                break;
                
            case MyHTML_TAG_B:
            case MyHTML_TAG_BIG:
            case MyHTML_TAG_BLOCKQUOTE:
            case MyHTML_TAG_BODY:
            case MyHTML_TAG_BR:
            case MyHTML_TAG_CENTER:
            case MyHTML_TAG_CODE:
            case MyHTML_TAG_DD:
            case MyHTML_TAG_DIV:
            case MyHTML_TAG_DL:
            case MyHTML_TAG_DT:
            case MyHTML_TAG_EM:
            case MyHTML_TAG_EMBED:
            case MyHTML_TAG_H1:
            case MyHTML_TAG_H2:
            case MyHTML_TAG_H3:
            case MyHTML_TAG_H4:
            case MyHTML_TAG_H5:
            case MyHTML_TAG_H6:
            case MyHTML_TAG_HEAD:
            case MyHTML_TAG_HR:
            case MyHTML_TAG_I:
            case MyHTML_TAG_IMG:
            case MyHTML_TAG_LI:
            case MyHTML_TAG_LISTING:
            case MyHTML_TAG_MENU:
            case MyHTML_TAG_META:
            case MyHTML_TAG_NOBR:
            case MyHTML_TAG_OL:
            case MyHTML_TAG_P:
            case MyHTML_TAG_PRE:
            case MyHTML_TAG_RUBY:
            case MyHTML_TAG_S:
            case MyHTML_TAG_SMALL:
            case MyHTML_TAG_SPAN:
            case MyHTML_TAG_STRONG:
            case MyHTML_TAG_STRIKE:
            case MyHTML_TAG_SUB:
            case MyHTML_TAG_SUP:
            case MyHTML_TAG_TABLE:
            case MyHTML_TAG_TT:
            case MyHTML_TAG_U:
            case MyHTML_TAG_UL:
            case MyHTML_TAG_VAR:
            case MyHTML_TAG_FONT:
            {
                // parse error
                if(token->tag_ctx_idx == MyHTML_TAG_FONT)
                {
                    myhtml_token_node_wait_for_done(token);
                    
                    if(myhtml_token_attr_by_name(token, "color", 5) == NULL &&
                       myhtml_token_attr_by_name(token, "face" , 4) == NULL &&
                       myhtml_token_attr_by_name(token, "size" , 4) == NULL)
                        return myhtml_insertion_mode_in_foreign_content_start_other(tree, token);
                }
                
                if(tree->fragment == NULL) {
                    myhtml_tree_node_t* current_node;
                    
                    do {
                        myhtml_tree_open_elements_pop(tree);
                        current_node = myhtml_tree_current_node(tree);
                    }
                    while(current_node && !(myhtml_tree_is_mathml_integration_point(tree, current_node) ||
                                            myhtml_tree_is_html_integration_point(tree, current_node) ||
                                            current_node->my_namespace == MyHTML_NAMESPACE_HTML));
                    
                    return true;
                }
            }
                
            default:
                return myhtml_insertion_mode_in_foreign_content_start_other(tree, token);
        }
    }
    
    return false;
}

void myhtml_rules_stop_parsing(myhtml_tree_t* tree)
{
    // THIS! IS! -(SPARTA!)- STOP PARSING
}

bool myhtml_rules_check_for_first_newline(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    if(tree->flags & MyHTML_TREE_FLAGS_PARSE_FLAG) {
        if(tree->flags &MyHTML_TREE_FLAGS_PARSE_FLAG_EMIT_NEWLINE)
        {
            if(token->tag_ctx_idx == MyHTML_TAG__TEXT) {
                myhtml_token_node_wait_for_done(token);
                
                if(token->length > 0) {
                    if(token->my_str_tm.data[0] == '\n') {
                        token->my_str_tm.data = mchar_async_crop_first_chars_without_cache(token->my_str_tm.data, 1);
                        
                        token->my_str_tm.length--;
                        token->length--;
                        
                        if(token->length == 0) {
                            tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_PARSE_FLAG);
                            return true;
                        }
                    }
                }
                else
                    return true;
            }
        }
        
        tree->flags ^= (tree->flags & MyHTML_TREE_FLAGS_PARSE_FLAG);
    }
    
    return false;
}

bool myhtml_rules_tree_dispatcher(myhtml_tree_t* tree, myhtml_token_node_t* token)
{
    // for textarea && pre && listen
    if(myhtml_rules_check_for_first_newline(tree, token))
        return false;
    
    if(tree->state_of_builder != MyHTML_TOKENIZER_STATE_DATA)
        tree->state_of_builder = MyHTML_TOKENIZER_STATE_DATA;
    
    bool reprocess = false;
    myhtml_tree_node_t* adjusted_node = myhtml_tree_adjusted_current_node(tree);
    
    if(tree->open_elements->length == 0 || adjusted_node->my_namespace == MyHTML_NAMESPACE_HTML) {
        reprocess = tree->myhtml->insertion_func[tree->insert_mode](tree, token);
    }
    else if(myhtml_tree_is_mathml_integration_point(tree, adjusted_node) &&
            ((token->tag_ctx_idx == MyHTML_TAG__TEXT ||
              (token->tag_ctx_idx != MyHTML_TAG_MGLYPH && token->tag_ctx_idx != MyHTML_TAG_MALIGNMARK)) &&
             (token->type & MyHTML_TOKEN_TYPE_CLOSE) == 0))
    {
            reprocess = tree->myhtml->insertion_func[tree->insert_mode](tree, token);
    }
    else if(adjusted_node->tag_idx == MyHTML_TAG_ANNOTATION_XML &&
       adjusted_node->my_namespace == MyHTML_NAMESPACE_MATHML &&
       token->tag_ctx_idx == MyHTML_TAG_SVG && (token->type & MyHTML_TOKEN_TYPE_CLOSE) == 0)
    {
        reprocess = tree->myhtml->insertion_func[tree->insert_mode](tree, token);
    }
    else if(myhtml_tree_is_html_integration_point(tree, adjusted_node) &&
            ((token->type & MyHTML_TOKEN_TYPE_CLOSE) == 0 || token->tag_ctx_idx == MyHTML_TAG__TEXT))
    {
        reprocess = tree->myhtml->insertion_func[tree->insert_mode](tree, token);
    }
    else if(token->tag_ctx_idx == MyHTML_TAG__END_OF_FILE)
        reprocess = tree->myhtml->insertion_func[tree->insert_mode](tree, token);
    else
        reprocess = myhtml_insertion_mode_in_foreign_content(tree, token);
    
    if(reprocess == false) {
        tree->token_last_done = token;
    }
    
    return reprocess;
}

myhtml_status_t myhtml_rules_init(myhtml_t* myhtml)
{
    myhtml->insertion_func = (myhtml_insertion_f*)mymalloc(sizeof(myhtml_insertion_f) * MyHTML_INSERTION_MODE_LAST_ENTRY);
    
    if(myhtml->insertion_func == NULL)
        return MyHTML_STATUS_RULES_ERROR_MEMORY_ALLOCATION;
    
    myhtml->insertion_func[MyHTML_INSERTION_MODE_INITIAL] = myhtml_insertion_mode_initial;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_BEFORE_HTML] = myhtml_insertion_mode_before_html;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_BEFORE_HEAD] = myhtml_insertion_mode_before_head;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_HEAD] = myhtml_insertion_mode_in_head;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_HEAD_NOSCRIPT] = myhtml_insertion_mode_in_head_noscript;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_AFTER_HEAD] = myhtml_insertion_mode_after_head;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_BODY] = myhtml_insertion_mode_in_body;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_TEXT] = myhtml_insertion_mode_text;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_TABLE] = myhtml_insertion_mode_in_table;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_TABLE_TEXT] = myhtml_insertion_mode_in_table_text;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_CAPTION] = myhtml_insertion_mode_in_caption;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_COLUMN_GROUP] = myhtml_insertion_mode_in_column_group;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_TABLE_BODY] = myhtml_insertion_mode_in_table_body;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_ROW] = myhtml_insertion_mode_in_row;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_CELL] = myhtml_insertion_mode_in_cell;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_SELECT] = myhtml_insertion_mode_in_select;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_SELECT_IN_TABLE] = myhtml_insertion_mode_in_select_in_table;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_TEMPLATE] = myhtml_insertion_mode_in_template;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_AFTER_BODY] = myhtml_insertion_mode_after_body;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_IN_FRAMESET] = myhtml_insertion_mode_in_frameset;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_AFTER_FRAMESET] = myhtml_insertion_mode_after_frameset;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_AFTER_AFTER_BODY] = myhtml_insertion_mode_after_after_body;
    myhtml->insertion_func[MyHTML_INSERTION_MODE_AFTER_AFTER_FRAMESET] = myhtml_insertion_mode_after_after_frameset;
    
    return MyHTML_STATUS_OK;
}


