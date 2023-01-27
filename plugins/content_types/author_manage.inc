<?php

$plugin = array(
  'single' => TRUE,
  'title' => t('Author Manage'),
  'description' => t('Manage author profiles and publications on orcid'),
  'category' => t('Scholarship'),
  #'edit form' => 'facultyscholarship_manage_author_form',
  'render callback' => 'facultyscholarship_manage_author_render',
  'admin info' => 'facultyscholarship_manage_author_form_info',
  'defaults' => array(),
  'all contexts' => TRUE,
);

function facultyscholarship_manage_author_render($subtype, $conf, $args, $contexts) {
  $author = node_load(array_pop($args));
  $constat = $author->field_faculty_orcid_connector[LANGUAGE_NONE][0]['value'];
  $oid = $author->field_faculty_orcid[LANGUAGE_NONE][0]['value'];
  $fsa = new FacultyScholarshipAdmin();
  $block = new stdClass();
  $block->id = 'fs_author_manage_block';
  $block->title = 'ORCID Synchronization Status';
  if( isset($oid) && $constat == 1) {
    $auid = $author->field_user_account[LANGUAGE_NONE][0]['target_id'];
    if(isset($auid)) {
      $account = user_load($auid);
      $fs = new FacultyScholarshipORCID();
      $orcid = new orcidAPI($fs->getUvaData($account->name));
      $name = $fsa->getName($orcid->id);
      $vars['orcid'] = $orcid;
      $empstat = $orcid->establishEmployment();
      $block->content = "";
      $oid = $name->field_faculty_orcid[LANGUAGE_NONE][0]['value'];
      if(isset($oid)){
        $block->content .= "<p><strong>Orcid Profile: </strong><a target='_blank' href='https://orcid.org/". $oid ."'>$oid</a></p>";
      }
      $block->content .= "<table>";
      if($empstat == TRUE) {
        $block->content .= "<tr><td><strong>Employment status:</strong></td><td>Current</td><td>";
        $vars['empstat'] = TRUE;
        $form = drupal_get_form('_fs_manage_author_orcid_emp_form', $vars);
        $block->content   .= drupal_render($form);
        $block->content .= "</td></tr>";
      } else {
        $block->content .= "<tr><td><strong>Employment status:</strong></td><td>Missing Law Affiliation</td><td>";
        $vars['empstat'] = FALSE;
        $form = drupal_get_form('_fs_manage_author_orcid_emp_form', $vars);
        $block->content   .= drupal_render($form);
        $block->content   .= "</td></tr>";
      }
      $block->content   .= "<tr><td><strong>Publication Catalog Status:</strong></td>";
      if(isset($name->field_faculty_orcid_pub_sync[LANGUAGE_NONE][0]['value'])){
        $block->content   .= "<td>Last full update: " . format_date($name->field_faculty_orcid_pub_sync[LANGUAGE_NONE][0]['value'], 'long') . "</td>";
      } else {
        $block->content   .= "<td>No synchronization has been executed.</td>";
      }
      $block->content   .= "<td>";
      $form = drupal_get_form('_fs_manage_author_orcid_pub_form', $vars);
      $block->content   .= drupal_render($form);
      $block->content .= "</td>";
      $block->content .= "<tr><td><strong>Publication Queue:</strong></td>";
      $queue = $fsa->worksQueue($author->nid);
      if(count($queue) > 0){
        $vars['queue'] = $queue;
        $block->content .= "<td><em>" . count($queue) . " works queued for ORCID:</em><ul>";
        foreach($queue as $work){
          $block->content .= "<li><a href='/node/" . $work['pid'] . "'> " . $work['work'] . "</a></li>";
        }
        $block->content .= "</ul></td>";
      } else {
        $vars['queue'] = FALSE;
        $block->content   .= "<td>All works are synced.</td>";
      }
      $block->content .= "<td>";
      $form = drupal_get_form('_fs_manage_author_orcid_queue_form', $vars);
      $block->content   .= drupal_render($form);


      $block->content   .= "</td></tr></table>";

    } else {
      $block->content = "An administrator must associate this faculty profile with an established user account in Drupal.";
    }

  } else {
    $block->content = "User has not registered with the UVA ORCID connector.";
  }

  return $block;
}

function _fs_manage_author_orcid_queue_form($form, $form_state){
  $form['orcid']['sync_queue'] = array(
    '#type' => 'submit',
    '#value' => t('Push Works Queue'),
    '#submit' => ['_fs_manage_author_orcid_queue_form_submit'],
  );
  if($form_state['build_info']['args'][0]['queue'] == FALSE){
    $form['orcid']['sync_queue']['#disabled'] = TRUE;
  }
  #dsm($form_state['storage']['responses']);
  return $form;
}

function _fs_manage_author_orcid_queue_form_submit($form, &$form_state){
  $orcid = $form_state['build_info']['args'][0]['orcid'];
  $queue = $form_state['build_info']['args'][0]['queue'];
  foreach($queue as $work){
    $response = $orcid->putWork(node_load($work['pid']), TRUE);
    dsm($response);
    $title = $work['work'];
    if(isset($response->{'developer-message'})) {
      $message = $response->{'developer-message'};
      drupal_set_message($title . ' | ' . $message, 'warning');
    }
  }
}

function _fs_manage_author_orcid_emp_form($form, $form_state){
  $form['orcid']['sync_empl'] = array(
    '#type' => 'submit',
    '#disabled' => $form_state['build_info']['args'][0]['empstat'],
    '#value' => t('Update Employment'),
    '#submit' => ['_fs_manage_author_orcid_emp_form_submit'],
  );
  return $form;
}
function _fs_manage_author_orcid_pub_form($form, $form_state){
  $form['orcid']['sync_pubs'] = array(
    '#type' => 'submit',
    '#value' => t('Update Entire Catalog'),
    '#submit' => ['_fs_manage_author_orcid_pub_form_submit'],
  );
  return $form;
}



function _fs_manage_author_orcid_emp_form_submit($form, &$form_state){
  $orcid = $form_state['build_info']['args'][0]['orcid'];
  $fsa = new FacultyScholarshipAdmin();
  $fsa->putIndividualEmployment($orcid);
}

function _fs_manage_author_orcid_pub_form_submit($form, &$form_state){
  $orcid = $form_state['build_info']['args'][0]['orcid'];
  $fsa = new FacultyScholarshipAdmin();
  $fsa->putIndividualCatalog($orcid, TRUE);
}
