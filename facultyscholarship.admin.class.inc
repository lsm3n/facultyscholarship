<?php



class FacultyScholarshipAdmin
{
  private $roles;

  function __construct(){
    $this->roles = [6486, 6471];
  }

  public function sync(){
    //sync employment and works data for UVA ORCID Connector registered faculty
    #$works = $this->worksQueue();
    foreach($this->employmentQueue() as $faculty){
      $fso = new FacultyScholarshipORCID();
      $orcid = new orcidAPI($fso->getUvaData($faculty['fid']));
      $orcid->establishEmployment();
    }
    foreach($this->worksQueue() as $work){
      $fso = new FacultyScholarshipORCID();
      $orcid = new orcidAPI($fso->getUvaData($work['fid']));
      $response = $orcid->putWork(node_load($work['pid']), TRUE);
      $title = $work['work'];
      if(isset($response->{'developer-message'})) {
        $message = $response->{'developer-message'};
        drupal_set_message($title . ' | ' . $message, 'warning');
      }
    }
  }

  public function checkUnregistered(){
    $list = $this->faculty(FALSE);
    $users = [];
    foreach($list as $faculty){
      if(isset($faculty['field_computing_id_value'])) {
        $orcid = new FacultyScholarshipORCID();
        $data = $orcid->getUvaData($faculty['field_computing_id_value']);
        if (isset($data->orcid_token)) {
          $users[] = $data->user_id;
        }
      }
    }
    return ['count'=> count($list), 'users' => $users];
  }

  function getFaculty(){
    foreach ($this->roles as $tid){
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'faculty')
        ->propertyCondition('status', NODE_PUBLISHED)
        #->fieldCondition('field_employee_type', 'tid', $tid)
        ->fieldCondition('field_faculty_orcid', 'value', NULL,"!=")
        #->addMetaData('account', user_load(1));
      ;
      $result = $query->execute();
      if (isset($result['node'])) {
        $scholars = entity_load('node', array_keys($result['node']));
      }
    }
    return $scholars;
  }

  function listIndividualWorks($fid){
    $fso = new FacultyScholarshipORCID();
    $orcid = new orcidAPI($fso->getUvaData($fid));
    return $orcid->getWorks();
  }

  function putIndividualEmployment($orcid){
    $orcid->putEmployment();
    $name = $this->getName($orcid->id);
    watchdog('facultyscholarship',
      "Faculty Scholarship | Employment Status Added for !name.",
      ['!name' => $name->title]
    );
  }

  function eraseIndividualCatalog($orcid){

  }

  function putIndividualCatalog($orcid, $force = FALSE){
    $fid = $orcid->id;
    $batch = array(
      'title' 			=> t('Synchronize Publications Data with ORCID'),
      'operations' 		=> array(),
      'init_message' 		=> t('Initializing...'),
      'progress_message' 	=> t('Operation @current out of @total.'),
      'error_message' 	=> t('an error occured.'),
    );

    foreach ($this->getFacultyCatalog($fid) as $work){
      $batch['operations'][] = $this->_fs_sendCatalog($orcid, $work, $force);
    }
    batch_set($batch);

    $name = $this->getName($fid);
    $name->field_faculty_orcid_pub_sync[LANGUAGE_NONE][0]['value'] = time();
    node_save($name);
    watchdog('facultyscholarship',
      "Faculty Scholarship | Entire Catalog Updated for !name.",
      ['!name' => $name->title]
    );
  }

  function _fs_sendCatalog($orcid, $work, $force){
    $orcid->putWork(node_load($work), $force);
  }

  function sendWork($publication){
    foreach ($publication->field_publication_author[LANGUAGE_NONE] as $key => $paragraph) {
      $authordata = array_pop(entity_load('paragraphs_item', [$paragraph['value']]));
      $author = node_load($authordata->field_publication_author_name[LANGUAGE_NONE][0]['target_id']);
      if (isset($author->field_faculty_orcid_connector[LANGUAGE_NONE]) && $author->field_faculty_orcid_connector[LANGUAGE_NONE][0]['value'] == 1) {
        $cid = $author->field_computing_id[LANGUAGE_NONE][0]['value'];
        $connector = new FacultyScholarshipORCID();
        $data = $connector->getUvaData($cid);
        $orcid = new orcidAPI($data);
        $orcid->putWork($publication, TRUE);
      }
    }
  }

  function reviewFormatted($publication){
    foreach ($publication->field_publication_author[LANGUAGE_NONE] as $key => $paragraph) {
      $paragraph = entity_load('paragraphs_item', [$paragraph['value']]);
      $authordata = array_pop($paragraph);
      $author = node_load($authordata->field_publication_author_name[LANGUAGE_NONE][0]['target_id']);
      if (isset($author->field_faculty_orcid_connector[LANGUAGE_NONE]) && $author->field_faculty_orcid_connector[LANGUAGE_NONE][0]['value'] == 1) {
        $cid = $author->field_computing_id[LANGUAGE_NONE][0]['value'];
        $connector = new FacultyScholarshipORCID();
        $data = $connector->getUvaData($cid);
        $orcid = new orcidAPI($data);
        return $orcid->formatWork($publication);
      }
    }
  }

  function getFacultyCatalog($fid){
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
      ->fieldCondition('field_computing_id', 'value', $fid, '=');
    $result = $query->execute();
    $keys = array_keys($result['node']);
    $nid = array_pop($keys);
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'paragraphs_item')
      ->fieldCondition('field_publication_author_name', 'target_id', $nid, '=');
    $works = $query->execute();
    $publications = [];
    foreach(array_keys($works['paragraphs_item']) as $work){
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'node')
        ->fieldCondition('field_publication_author', 'value', $work, '=');
      $result = $query->execute();
      $publications[] = array_pop(array_keys($result['node']));
    }
    return $publications;
  }

  function checkLocalPutcode($putcode){
    $q = db_select('field_data_field_publication_orcid_putcode', 'pc');
    $q->condition('pc.field_publication_orcid_putcode_value', $putcode, '=');
    $q->fields('pc', ['entity_id']);
    $results = $q->execute();

    foreach ($results as $result) {
      return $result->entity_id;
    }
    return FALSE;
  }

  function getName($fid){
    $q = db_select('field_data_field_computing_id', 'fid');
    $q->leftJoin('node', "n", 'n.nid = fid.entity_id');
    $q->condition('fid.field_computing_id_value', $fid, '=');
    $q->fields('n', ['nid']);
    foreach ($q->execute() as $result) {
      return node_load($result->nid);
    }
    return FALSE;
  }

  function faculty($registered = FALSE){
    $q = db_select('node', 'n');
    $q->leftJoin('field_data_field_computing_id', 'fid', 'fid.entity_id = n.nid');
    $q->fields('fid', ['field_computing_id_value']);
    $q->leftJoin('field_data_field_faculty_orcid', 'orcid', 'orcid.entity_id = n.nid');
    $q->fields('orcid', ['field_faculty_orcid_value']);
    $q->leftJoin('field_data_field_employee_type', 'et', 'et.entity_id = n.nid');
    $q->fields('et', ['field_employee_type_tid']);
    $q->leftJoin('field_data_field_faculty_orcid_employment', 'emppc', 'emppc.entity_id = n.nid');
    $q->fields('emppc', ['field_faculty_orcid_employment_value']);
    $q->leftJoin('field_data_field_faculty_orcid_pub_sync', 'ops', 'ops.entity_id = n.nid');
    $q->fields('ops', ['field_faculty_orcid_pub_sync_value']);
    $or_type = db_or();
    foreach($this->roles as $role){
      $or_type->condition('et.field_employee_type_tid', $role, '=');
    }
    if ($registered == FALSE){
      $q->isNull('orcid.field_faculty_orcid_value');
    } else {
      $q->isNotNull('orcid.field_faculty_orcid_value');
    }
    $q->fields('n', ['nid', 'title']);
    $q->condition('n.type', 'faculty', '=')
      ->condition($or_type)
      ->condition('status', 1);
    $row = $q->execute();
    $scholars = [];
    while($scholar = $row->fetchAssoc()){
      $scholars[] = $scholar;
    }
    return $scholars;
  }

  function employmentQueue(){
    $q = db_select('node', 'author');
    $q->leftJoin('field_data_field_faculty_orcid_connector', 'connector', 'author.nid = connector.entity_id');
    $q->leftJoin('field_data_field_faculty_orcid_employment', 'employment', 'author.nid = employment.entity_id');
    $q->leftJoin('field_data_field_computing_id', 'fid', 'author.nid = fid.entity_id');
    $q->fields('author', ['nid', 'title']);
    $q->addField('fid', 'field_computing_id_value', 'fid');
    $q->condition('type', 'faculty');
    $q->condition('connector.field_faculty_orcid_connector_value', 1, '=');
    $q->isNull('employment.field_faculty_orcid_employment_value');
    $row = $q->execute();
    while($scholar = $row->fetchAssoc()){
      $scholars[] = $scholar;
    }
    return $scholars;
  }

  function worksQueue($faculty = FALSE){
    $q = db_select('field_data_field_publication_author_name', 'authorship');
    $q->addField('authorship', 'field_publication_author_name_target_id', 'aid');
    $q->leftJoin('node', 'author', 'authorship.field_publication_author_name_target_id = author.nid');
    $q->addField('author', 'title', 'author');
    $q->leftJoin('field_data_field_publication_author', 'authorpara', 'authorpara.field_publication_author_value = authorship.entity_id');
    $q->leftJoin('field_data_field_computing_id', 'fid', 'author.nid = fid.entity_id');
    $q->addField('fid', 'field_computing_id_value', 'fid');
    $q->leftJoin('field_data_field_faculty_orcid', 'forc', 'author.nid = forc.entity_id');
    $q->addField('forc', 'field_faculty_orcid_value', 'orcid');
    $q->leftJoin('node', 'pub', 'authorpara.entity_id = pub.nid');
    $q->addField('pub', 'nid', 'pid');
    $q->addField('pub', 'changed', 'local_modified');
    $q->addField('pub', 'title', 'work');
    $q->leftJoin('field_data_field_publication_orcid_putcode', 'pc', 'pub.nid = pc.entity_id');
    $q->addField('pc', 'field_publication_orcid_putcode_value', 'orcid_pc');
    $q->leftJoin('field_data_field_publication_orcid_modified', 'modified', 'pub.nid = modified.entity_id');
    $q->addField('modified', 'field_publication_orcid_modified_value', 'orcid_modified');
    $q->leftJoin('field_data_field_publication_orcid_issue', 'issue', 'pub.nid = issue.entity_id');
    $q->addField('issue', 'field_publication_orcid_issue_value', 'issue');
    $q->leftJoin('field_data_field_faculty_orcid_connector', 'connector', 'author.nid = connector.entity_id');
    $or = db_or();
      $or->where("pub.changed > modified.field_publication_orcid_modified_value");
      $or->isNull("pc.field_publication_orcid_putcode_value");
      $or->isNull('modified.field_publication_orcid_modified_value');
    $q->condition($or);
    $q->condition('author.status', 1, '=');
    $q->condition('connector.field_faculty_orcid_connector_value', 1, '=');
    if($faculty != FALSE){
      $q->condition('authorship.field_publication_author_name_target_id', $faculty, '=');
    }
    $row  = $q->execute();
    while($work = $row->fetchAssoc()){
        $queue[] = $work;
    }
    return $queue;
  }

  /*
   * List all works on orcid that are not registered by putcode locally
   *
   */
  function unregisteredOrcidWorks(){
    foreach($this->listAllOrcidWorks(TRUE) as $faculty) {
      unset($unregistered);
      foreach($faculty['works'] as $key => $work){
        $status = $this->checkLocalPutcode($key);
        if($status == FALSE){
          $unregistered[$key] = $work;
        }
      }
      if(isset($unregistered)){
        $all[] = [
          'faculty' => $faculty['data'],
          'unregistered' => $unregistered
        ];
      }
    }
    return $all;
  }

  function listAllOrcidWorks($externalcheck = FALSE){
    $registered = $this->faculty(TRUE);
    foreach($registered as $faculty){
      $connector = new FacultyScholarshipORCID();
      $data = $connector->queryConnector($faculty['field_computing_id_value']);
      $orcid_api = new orcidAPI($data);
      $works = $orcid_api->getWorks();
      if(count($works ) > 0 ){
        if($externalcheck == TRUE){
          $all[$faculty['title']] = [
            'data' => $faculty,
            'works' => $works
          ];
        } else {
          $all[$faculty['title']] = $works;
        }
      }
    }
    return $all;
  }

  function queueAuthorWorks(){
    $authors = [];
    foreach( $this->worksQueue() as $work) {
      $aid = $work['aid'];
      $authors[$aid][] = $work;
    }
    return $authors;
  }

  function queueCounter(){
    $counts = [];
    foreach( $this->worksQueue() as $work){
      $pid = $work['pid'];
      if(isset($counts[$pid])){
        $counts[$pid] = $counts[$pid] + 1;
      } else {
        $counts[$pid] = 1;
      }
    };
    dsm($counts);
  }


   function cleanup()
   {
     // select all publications where ALL associated authors are unpublished
     // find all unpublished fac/staff
     // list all publications associated
     // confirm all authors are unpublished
     // batch final list to unpublished
     dsm('yop');
     $query = new EntityFieldQuery();
     $query->entityCondition('entity_type', 'node')
       ->entityCondition('bundle', 'faculty')
       ->propertyCondition('status', 0);
     $list = $query->execute();

         foreach($list['node'] as $nid => $faculty){

            $query = new EntityFieldQuery();
           $query->entityCondition('entity_type', 'paragraphs_item')
             ->entityCondition('bundle', 'publication_author')
             ->fieldCondition('field_publication_author_name', $nid);
           $publications[$nid] = $query->execute();

           dsm($publications);
break;
         }


   }

  function protectAuthors(){

    ///disallow delete of faculty node if publications exist
    /// OR
    /// convert author field to n
  }

function migrateCoAuthsToNameField(){

}

}

