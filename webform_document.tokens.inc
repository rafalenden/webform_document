<?php

/**
 * Implements hook_token_info().
 */
function webform_document_token_info() {
  $tokens = array();

  $tokens['tokens']['webform']['checkboxes-(Field_Key)'] = array(
    'name' => t('Reders as checkboxes'),
    'description' => t('Render select component options as checkboxes.'),
  );

  return $tokens;
}

/**
 * Implements hook_tokens().
 */
function webform_document_tokens($type, array $tokens, array $data = array(), array $options = array()) {
  if ($type === 'webform' && isset($data['webform']) && is_object($data['webform'])) {
    $replacements = array();
    $sanitize = !empty($options['sanitize']);

    $submission = $data['webform'];
    $nid = $submission->nid;
    $sid = $submission->sid;

    foreach ($tokens as $name => $original) {
      static $components;
      if (!isset($components[$nid])) {
        $components = array($nid => array($sid => array()));

        $query = "SELECT * FROM {webform_component}
          WHERE type = 'select' AND nid = :nid";
        $results = db_query($query, array(
          ':nid' => $submission->nid,
         ))->fetchAllAssoc('cid', PDO::FETCH_ASSOC);

        foreach ($results as $this_component) {
          $cid = $this_component['cid'];
          if (isset($submission->data[$cid])) {
            $submission->data[$cid] += $this_component;
          }
          else {
            $submission->data[$cid] = array();
          }
          $components[$nid][$sid] += array($cid => $submission->data[$cid]);
        }
      }
      $cid_found = FALSE;
      foreach ($components as $this_nid => $this_submission) {
        foreach ($this_submission as $this_sid => $this_component) {
          foreach ($this_component as $this_cid => $component_data) {
            if (!empty($component_data)) {
              if ($name == 'checkboxes-' . $component_data['form_key']) {
                $cid = $this_cid;
                $cid_found = TRUE;
                break 3;
              }
            }
          }
        }
      }
      if ($cid_found && !empty($components[$nid][$sid][$cid])) {
        $replacements += _webform_document_add_component_replacement(
         $components[$nid][$sid][$cid], $submission, array(
          'original' => $original,
          'name' => $name,
          'tokens' => $tokens,
          'options' => $options,
         )
        );
      }
    }

    return $replacements;
  }
}

/**
 * Helper function to find an individual replacement.
 */
function _webform_document_add_component_replacement($component, $submission, $token_data) {
  $replacements = array();
  $nid = $submission->nid;
  $sid = $submission->sid;

  // First, unserialize everything so we can work with them as arrays.
  $token_value = FALSE;
  // For components with selectable options (including grids), make an array of options in the form array(safe_key => "Some readable option", ...)
  $options = FALSE;
  if (isset($component['extra']) && is_string($component['extra'])) {
    $component['extra'] = unserialize($component['extra']);
    // Selects use "items".
    if (isset($component['extra']['items']) && is_string($component['extra']['items'])) {
      $component['extra']['options'] = trim($component['extra']['items']);
    }
    // Grids use "options".
    if (isset($component['extra']['options']) && is_string($component['extra']['options'])) {
      foreach (preg_split('/[\r\n]+/', $component['extra']['options']) as $option) {
        if (strpos($option, '|') !== FALSE) {
          $option = explode('|', $option, 2);
          $options[(string) $option[0]] = $option[1];
        }
        else {
          // Make sure the keys are strings.
          $options[(string) $option] = $option;
        }
      }
    }
  }

  if (!empty($component['value']) && is_array($component['value']) && $component['type'] == 'select') {
    $token_value = array();
    $no = 1;
    foreach ($options as $key => $option) {
      $char = in_array($key, $component['value']) ? '☒' : '☐'; // ☑
      $value = $char . '  ' . $option;
      // Replace regular spaces with non-braking spaces (nbsp).
      $value = str_replace(' ', '&#xA0;', $value);
      $token_value[] = $value;
      $no++;
    }
    $token_value = implode('  ', $token_value);
  }
  else {
    $token_value = '';
  }

  $replacements[$token_data['original']] = $token_value;
  return $replacements;
}
