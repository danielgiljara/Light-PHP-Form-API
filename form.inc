<?php

/**
 * @file
 * Lightweight implementation of a general purpose API for form management.
 *
 * This is inspired in other projects. See attached links.
 *
 * @todo Complete the documentation.
 *
 * @author Daniel Gil Jara (danielgiljara@gmail.com)
 *
 * @see http://drupal.org/node/751826
 * @see http://code.google.com/p/phpformapi/
 * @see http://freshmeat.net/projects/htmlformsgenerationandvalidation
 * @see http://www.palmsform.garyc40.com/documentation/
 * @see http://api.cakephp.org/class/form-helper
 * @see https://github.com/lemurframework/lemurframework/blob/master/core/form.class.php
 *
 * @defgroup forms Form management functions
 * @{
 */

function form_get_form() {
  // Get form name from the first argument.
  $args = func_get_args();
  $form_name = array_shift($args);

  // Get form from the proper function passing the rest of arguments.
  $form = call_user_func_array($form_name, $args);

  // Prepare some basic data of form and process.
  form_form_prepare($form_name, $form);
  form_form_process($form_name, $form);

  return form_form_render($form_name, $form);
}

function form_form_prepare($form_name, &$form) {
  // Attach basic data.
  $form['#type'] = 'form';
  $form += array(
    '#id' => form_clean_id($form_name),
    '#action' => '',
    '#method' => 'POST',
  );

  // Attach a form build id.
  $form['#build_id'] = form_form_build_id($form['#id']);
  $form['build_id'] = array(
    '#type' => 'hidden',
    '#value' => $form['#build_id'],
  );
}

/**
 * Process a form.
 */
function form_form_process($form_name, &$form) {
  if (isset($_POST['build_id']) && $_POST['build_id'] === form_form_build_id($form['#id'])) {
    // Save POST data into form values.
    form_form_populate($form_name, $form);

    // Validate and submit.
    if (form_form_validate($form_name, $form)) {
      form_form_submit($form_name, $form);
    }
  }
}

function form_form_populate($form_name, &$form) {
  foreach (element_children($form) as $children_name) {
    if (isset($_POST[$children_name]) && !$form[$children_name]['#type'] !== 'markup') {
      $form[$children_name]['#value'] = $_POST[$children_name];
    }
    form_form_populate($form_name, $form[$children_name]);
  }
}

function form_form_build_id($form_name) {
  return 'form-' . md5($form_name);
}

function form_form_validate($form_name, &$form) {
  $validate = $form_name . '_validate';
  if (function_exists($validate)) {
    if ($validate($form)) {
      return TRUE;
    }
    return FALSE;
  }
  return TRUE;
}

function form_form_submit($form_name, &$form) {
  $submit = $form_name . '_submit';
  if (function_exists($submit)) {
    return $submit($form);
  }
  return TRUE;
}

/**
 * Render a form element.
 */
function form_form_render($name, $elements) {
  $content = '';

  $attributes = array();

  // Set base attributes from #attributes.
  if (!empty($elements['#attributes'])) {
    $attributes = $elements['#attributes'];      
  }

  $elements['#id'] = isset_or($elements['#id'], form_clean_id('edit-' .$name));

  // Set tag name.
  $attributes['name'] = $name;

  // Transfer some properties to tag attributes if they not are previously
  // declared.
  foreach (array('class', 'style', 'method', 'action', 'id') as $key => $attribute) {
    if (isset($elements['#' . $attribute])) {
      $attributes += array(
        $attribute => $elements['#' . $attribute],
      );
    }
  }

  // Set required and error classes.
  foreach (array('required', 'error') as $key => $name) {
    if (isset($elements['#' . $name])) {
      form_append_class($attributes, $name);
    }
  }

  // Date fields are like text fields except they have the 'date' class
  // appended and have a default size of 10.
  if ($elements['#type'] === 'date') {
    form_append_class($attributes, 'date');
    $elements['#size'] = isset_or($elements['#size'], 10);
    $elements['#type'] = 'text';
  }

  // Get field value or default instead.
  $field_value = isset_or($elements['#value'], isset_or($elements['#default_value'], ''));

  $content .= isset_or($elements['#prefix'], '');

  if (isset($elements['#title'])) {
    $content .= '<label for="' . $attributes['id'] . '">' . $elements['#title'] . ':</label> ';
  }

  // Perform different actions in order to construct each type of element.
  switch ($elements['#type']) {

    // Select list.
    case 'select':

      // Switch to multiple select mode.
      if (isset($elements['#multiple'])) {
        $attributes['multiple'] = 'multiple';
        $attributes['name'] .= '[]';
      }

      $content .= '<select' . form_tag_attributes($attributes) . '>';

      // Run trought options. When finds an option that matches one or more
      // of field values mark it as selected.
      foreach ($elements['#options'] as $value_key => $value_label) {
        $attributes = array(
          'value' => $value_key == 0 ? '' : $value_key,
        );
        if (isset($field_value)) {
          if ((is_array($field_value) && in_array($value_key, $field_value)) || $value_key == $field_value) {

            $attributes['selected'] = 'selected';
          }
        }
        $content .= '<option' . form_tag_attributes($attributes) . '>' . $value_label . '</option>';
      }

      $content .= '</select>';
      break;

    // Other elements.
    case 'button':
    case 'submit':
    case 'reset':
      form_append_class($attributes, 'button');
    case 'file':
    case 'hidden':
    case 'image':
    case 'password':
    case 'date':
    case 'text':
      $attributes['size'] = isset_or($elements['#size']);

      if ($elements['#type'] === 'text' && isset($elements['#multiple'])) {
        $attributes['rows'] = isset_or($elements['#size']);
        $content .= '<textarea' . form_tag_attributes($attributes) . '>' . isset_or($field_value, '') . '</textarea>';
      }

      else {
        // Transfer more values to tag attributes and construct the tag.
        $attributes += array(
          'size' => $elements['#size'],
          'type' => $elements['#type'],
          'value' => isset_or($field_value),
        );
        $content .= '<input' . form_tag_attributes($attributes) . ' />';
      }

      break;

    // Renders raw html to page.
    case 'markup':
      $content .= $field_value;
      break;

    // Wrapper tags.
    case 'form':
      $wrapper_tag = 'form';
      break;
    case 'wrapper':
      $wrapper_tag = 'div';
  }

  // Open wrapper tags.
  if (isset($wrapper_tag)) {
    $content .= "<$wrapper_tag" . form_tag_attributes($attributes) . '>';
  }

  // Attach children elements to content.
  foreach (element_children($elements) as $children_name) {
    $content .= form_form_render($children_name, $elements[$children_name]);
  }

  // Close wrapper tags.
  if (isset($wrapper_tag)) {
    $content .= "</$wrapper_tag>";
  }

  $content .= isset_or($elements['#suffix'], '');

  if (isset($elements['#error'])) {
    $content .= '<label class="error">' . $elements['#error'] . '</label>';
  }

  return $content;
}

/**
 * Append class to an array of tag attributes.
 *
 * @param $attributes
 *   The array with tag attributes. Class will be appended to the array element
 *   with key 'class'.
 * @param $class_name
 *   An string with the class to add.
 *
 * @see form_tag_attributes()
 */
function form_append_class(&$attributes, $class_name) {
  if (!isset($attributes['class'])) {
    $attributes['class'] = $class_name;
  }
  elseif (!preg_match('/\b' . $class_name . '\b/', $attributes['class'])) {
    $attributes['class'] .= " $class_name";
  }
}

function isset_or(&$variable, $default = NULL) {
  return isset($variable) ? $variable : $default;
}

/**
 * Get keys of a structured array tree element that are not properties (i.e., do not begin with '#').
 *
 * This function is borrowed from Drupal.
 */
function element_children($element) {
  return array_filter(array_keys((array) $element), 'element_child');
}

/**
 * Check if the key is a child.
 *
 * This function is borrowed from Drupal.
 */
function element_child($key) {
  return !isset($key[0]) || $key[0] != '#';
}

/**
 * Prepare an HTML ID attribute string for a form item.
 *
 * Remove invalid characters and guarantee uniqueness.
 *
 * This function is borrowed from Drupal.
 *
 * @param $id
 *   The ID to clean.
 * @param $flush
 *   If set to TRUE, the function will flush and reset the static array
 *   which is built to test the uniqueness of element IDs. This is only
 *   used if a form has completed the validation process. This parameter
 *   should never be set to TRUE if this function is being called to
 *   assign an ID to the #ID element.
 *
 * @return
 *   The cleaned ID.
 */
function form_clean_id($id = NULL, $flush = FALSE) {
  static $seen_ids = array();

  if ($flush) {
    $seen_ids = array();
    return;
  }
  $id = str_replace(array('][', '_', ' '), '-', $id);

  // Ensure IDs are unique. The first occurrence is held but left alone.
  // Subsequent occurrences get a number appended to them. This incrementing
  // will almost certainly break code that relies on explicit HTML IDs in
  // forms that appear more than once on the page, but the alternative is
  // outputting duplicate IDs, which would break JS code and XHTML
  // validity anyways. For now, it's an acceptable stopgap solution.
  if (isset($seen_ids[$id])) {
    $id = $id .'-'. $seen_ids[$id]++;
  }
  else {
    $seen_ids[$id] = 1;
  }

  return $id;
}
/**
 * @} End of "defgroup forms".
 */
