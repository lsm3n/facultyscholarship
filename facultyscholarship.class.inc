<?php


use Seboettg\CiteProc\StyleSheet;
use Seboettg\CiteProc\CiteProc;

class facultyscholarship
{
  protected $publication;
  protected $type;
  protected $bibData;
  protected $style;
  protected $stylesheet;
  protected $map;
  protected $bibAuthor;
  protected $hideTitle;
  var $suppress;
  var $single;
  protected $sourceMap;

  function __construct($publication)
  {
    $this->publication = entity_metadata_wrapper('node', $publication);
    $this->suppress = TRUE;
    $this->getPublicationType();
    $this->showAuthor();
  }

  private function getPublicationType(){
    $this->type = $this->publication->field_publication_type->value()->name;
  }

  private function typeMap(){
    $typeMap = [
      "Law Review Article" => "article-journal",
      "Book" => "book",
      "Book Review" => "review",
      "Book Chapter" => "chapter",
      "Textbook" => "book",
      "Editorial/Opinion" => "article-newspaper",
      "Journal Article" => "article",
      "Forthcoming" => 'document',
      "Governmental Testimony" => 'report',
      "White Paper" => 'report',
      "Brief" => 'report',
      "Report" => 'report',
      "Working Paper" => 'working-paper',
      "Work in Progress" => 'working-paper',
      "Bundle" => 'article-journal'
    ];

    return $typeMap[$this->type];
  }

  function showAuthor(){
    $this->suppress = FALSE;
  }

  function hideAuthor(){
    $this->suppress = TRUE;
  }

  function publicationsList($author){
    $this->hideAuthor();
    $this->hideTitle = TRUE;
    $this->bibAuthor = $author;
    $this->map();
    $this->loadCustomStylesheet('uvalaw-custom');
   # $this->loadCustomStylesheet('apa');
    $CiteProc = new CiteProc($this->stylesheet);
    return $CiteProc->render(json_decode($this->map), "citation");
  }

  function getMap(){
    $this->map();
    return $this->map;
  }

  function test(){
    $this->single = TRUE;
    $this->map();

    $metadata = drupal_get_path('module', 'facultyscholarship') . '/metadata.json';
    $data = json_decode(file_get_contents($metadata));
    $this->loadCustomStylesheet('uvalaw-custom-authors');
    #$this->loadCustomStylesheet('apa');
    #$this->loadDefaultStylesheet('uvalaw-custom-authors');
    $CiteProc = new CiteProc($this->stylesheet);


    return $CiteProc->render($data, "citation");
  }

  function citation(){
    $this->single = TRUE;
    $this->map();
    #$this->loadDefaultStylesheet('bluebook-law-review');
    $this->loadCustomStylesheet('uvalaw-custom-authors');
    #$this->loadCustomStylesheet('apa');
    $CiteProc = new CiteProc($this->stylesheet);
    return $CiteProc->render(json_decode($this->map), "citation");
  }

  function authorslist(){
    $this->bibdata();
    $authors = $this->authors();
    $map = json_encode([
      $authors['type'] => $authors['authors'],
      'type' => $this->typeMap(),
    ]);
    $this->loadCustomStylesheet('uvalaw-custom-authors');
    $CiteProc = new CiteProc($this->stylesheet);
    $list =  $CiteProc->render([json_decode($map)], "citation");

    #return $list;
    return str_replace(', .', '', $list);

  }

  function source(){
    $this->single = TRUE;
    $this->sourceMap();
    #$this->loadDefaultStylesheet('bluebook-law-review');
    $this->loadCustomStylesheet('uvalaw-custom-authors');
    #$this->loadCustomStylesheet('apa');
    $CiteProc = new CiteProc($this->stylesheet);

    return $CiteProc->render(json_decode($this->sourceMap), "citation");
  }

  private function loadCustomStylesheet($style){
    $stylePath = self::localStylePath() . $style . '.csl';
    if(file_exists($stylePath)){
      $this->stylesheet = file_get_contents($stylePath);
    }
  }

  private function loadDefaultStylesheet($style){

    $this->stylesheet = StyleSheet::loadStyleSheet($style);
  }

  function load(){
    return $this->publication;
  }

  private static function localStylePath()
  {
    $module_path = drupal_get_path('module', 'facultyscholarship');
    return $module_path . '/csl/';
  }

  function title(){
    $title = $this->publication->field_publication_title->value();
    if ($this->hideTitle != TRUE){
      return  html_entity_decode(trim(strip_tags($title['value'], '<em>')));
    } else {
      return FALSE;
    }
  }

  private function bibdata(){
    $bibData = $this->publication->field_publication_data->value();

    $this->bibData = entity_metadata_wrapper('paragraphs_item', $bibData[0]->item_id); //array_shift



  }

  private function publisher() {
    $publisherTerm = $this->bibData->field_publication_publisher->value();
      if (isset($publisherTerm)) {
        $publisherTerm = $this->bibData->field_publication_publisher->value();
/* if using abbreviations for law reviews publishers
 *         $publisherParagraph = entity_metadata_wrapper('taxonomy_term', $publisherTerm->tid);

        if ($publisherParagraph->field_pulisher_abbreviation->value() != '') {
          return $publisherParagraph->field_pulisher_abbreviation->value();
        }
  */
        return $publisherTerm->name;
      }
      else {
        return FALSE;
      }
  }

  private function container(){
    $container =  $this->getDataField($this->bibData, 'field_publication_book_title');
    if($container){
      return html_entity_decode(trim(strip_tags($container['value'])));
    }
    #return $this->bibData->field_publication_book_title->value();

  }

  private function pubplace(){
    $publisherTerm = $this->bibData->field_publication_publisher->value();
    if($publisherTerm) {
      $publisherParagraph = entity_metadata_wrapper('taxonomy_term', $publisherTerm->tid);
      if ($publisherParagraph->__isset('field_publication_location')) {
        return $publisherParagraph->field_publication_location->value();
      }
    } else {
      return FALSE;
    }
  }

  function getAuthors(){
    $this->suppress = FALSE;
    return $this->authors();
  }


  function authors(){
    $authors = [];
    $editorStatus = FALSE;
    foreach ($this->publication->field_publication_author->value() as $authorData) {
      $authorParagraph = entity_metadata_wrapper('paragraphs_item', $authorData->item_id);
      $authorNid = $authorParagraph->field_publication_author_name->value();
      $authorRole = $authorParagraph->field_publication_author_role->value();
      $pubtype = $this->publication->field_publication_type->value()->name;
      if($authorRole == 'editor' && $editorStatus == FALSE){
        $editorStatus = TRUE;
      }
      $authorNode = entity_metadata_wrapper('node', $authorNid[0]->nid);
      $external = $authorParagraph->field_publication_external_name->value();
      unset($nameParts);
      if ($authorNode->getIdentifier() != FALSE) {
        $nameParts["family"] = $authorNode->field_last_name->value();
        $nameParts["given"] = $authorNode->field_first_name->value();
        if ($authorNid[0]->nid != $this->bibAuthor) {
          $authors[] = $nameParts;
        }
        if ($authorParagraph->getIdentifier() == $this->bibAuthor && $this->suppress == FALSE) {
          $authors[] = $nameParts;
        }
      } elseif(isset($external)){
          $nameParts["family"] = $external['family'];
          $nameParts["given"] = rtrim($external['given'] . " " .$external['middle']);
          $authors[] = $nameParts;
      }
    }
      /*
       * foreach ($this->publication->field__publication_coauthor->value() as $coauthorData) {
        $nameParts = explode(', ', $coauthorData);
        $parts = [];
        $parts['family'] = $nameParts[0];
        if($nameParts[1]) {
          $parts['given'] = $nameParts[1];
        }
        $authors[] = $parts;
      }
*/
      $authorType = ($editorStatus == TRUE ? 'editor' : 'author');
      $sortMethod = $this->publication->field_publication_author_sort->value();
      switch($sortMethod){
        case "manual":
          return ['type' => $authorType, 'authors' => $authors];
        default:
          array_multisort(
            array_map(function($element){
              if(isset($element[0])){
                return $element[0]['family'];
              }
            }, $authors), SORT_ASC, $authors);
          return ['type' => $authorType, 'authors' => $authors];


      }

  }

  private function editor(){
    $data =  $this->getDataField($this->bibData, 'field_publication_book_editor');
    $editors = [];
    if($data) {
      foreach ($data as $editor) {
        $editors[] = ['family' => $editor];
      }
      return $editors;
    } else {
      return FALSE;
    }
  }

  private function getDataField($paragraph, $field){
    try {
      return $paragraph->$field->value();
    }
    catch (EntityMetadataWrapperException $e) {
      return FALSE;
    }

    /*
      if($paragraph->__isset($field)){
          return $paragraph->$field->value();
      } else {
          return NULL;
      }
    */
  }

  private function edition(){
    $edition = $this->getDataField($this->bibData, 'field_publication_edition');
    $altEditionLabel = $this->getDataField($this->bibData, 'field_publication_edition_alt');
    if(!isset($edition) && isset($altEditionLabel)) $edition = $altEditionLabel;
    if($this->single == TRUE){
      return $edition;
    } else {
      $multiedition = $this->publication->field_publication_edition_bundle->value();
      if ($multiedition) {
        $primary = $multiedition->nid;
        $results = db_query(
          "SELECT edition.field_publication_edition_value as edition
                FROM {field_data_field_publication_edition_bundle} initial
                JOIN {field_data_field_publication_data} pubdata on initial.entity_id = pubdata.entity_id
                JOIN {field_data_field_publication_edition} edition on pubdata.field_publication_data_value = edition.entity_id
                WHERE initial.field_publication_edition_bundle_target_id = :target
                ORDER BY edition.field_publication_edition_value ASC", [':target' => $primary]
        );
        $editions = [];
        foreach ($results as $result) {
          $editions[] = $result->edition;
        }
        return implode(", ", $this->getRanges($editions));
      }
      elseif (isset($edition)) {
        return $edition;
      }
      return FALSE;
    }
  }

  private function issued(){
    if($this->publication->field_publication_edition_bundle->value()) {
      $primary = $this->publication->field_publication_edition_bundle->value()->nid;
    }
    if(!isset($primary) || $this->single == TRUE){
      $paragraph = $this->publication->field_publication_data->value();
      $dates = $paragraph[0]->field_publication_dates[LANGUAGE_NONE][0]['from'];
      $parts = [];
      if ($dates['year']) $parts[] = $dates['year'];
      if ($dates['month']) $parts[] = $dates['month'];
      if ($dates['day']) $parts[] = $dates['day'];
      if($parts) {
        $issued = [
          "date-parts" => [$parts]
        ];
        return $issued;
      }
    } else {
      $edition = $this->getDataField($this->bibData, 'field_publication_edition');
      $primary = $this->publication->field_publication_edition_bundle->value()->nid;
      if ($primary) {
        $results = db_query(
          "SELECT dates.field_publication_dates_year as years
                FROM {field_data_field_publication_edition_bundle} initial
                JOIN {field_data_field_publication_data} pubdata on initial.entity_id = pubdata.entity_id
                JOIN {field_data_field_publication_dates} dates on pubdata.field_publication_data_value = dates.entity_id
                WHERE initial.field_publication_edition_bundle_target_id = :target
                ORDER BY dates.field_publication_dates_year ASC", [':target' => $primary]
        );
        $years = [];
        foreach ($results as $result) {
          $years[] = $result->years;
        }
        if (count($years) > 1) {
          $first = array_shift($years);
          $last = array_pop($years);
          $range["date-parts"][][] = $first;
          $range["date-parts"][][] = $last;
          return $range;
        } else {
          $single["date-parts"][][] = $years[0];
          return $single;
        }
      }
    }
  }


  private function reviewed_title(){
    $reviewPara = $this->getDataField($this->bibData, 'field_publication_bookreview');
    if(isset($reviewPara->field_publication_review_title)) {
      return html_entity_decode(trim(strip_tags($reviewPara->field_publication_review_title[LANGUAGE_NONE][0]['value'], '<em>')));
    }
  }

  private function reviewed_author(){
    $reviewPara = $this->getDataField($this->bibData, 'field_publication_bookreview');
    if(isset($reviewPara->field_publication_review_author)) {
      return trim($reviewPara->field_publication_review_author[LANGUAGE_NONE][0]['value']);
    }
  }

  private function sourceMap(){
    $this->bibData();
    $map[] = [
      'type'                      => $this->typeMap(),
      'reviewed-title'            => $this->reviewed_title(),
      'reviewed-author'           => $this->reviewed_author(),
      'publisher'                 => $this->publisher(),
      'publisher-place'           => $this->pubplace(),
      'collection-editor'         => $this->editor(),
      'issued'                    => $this->issued(),
      "container-title"           => $this->container(),
      'volume'                    => $this->getDataField($this->bibData, 'field_publication_volume'),
      'page'                      => $this->getDataField($this->bibData, 'field_publication_pagination'),
      'editor'                    => $this->editor(),
      'issue'                     => $this->getDataField($this->bibData, 'field_publication_issue'),
      'edition'                   => $this->edition(),
    ];
    $this->sourceMap = json_encode($map);
  }



  private function map() {
    $this->bibdata();
    $authors = $this->authors();
    #dsm($authors);
    #dsm($this->editor());
    #dsm($authors);
    $map[] = [
      'id' => 'law-fs-565656',
      'type'                      => $this->typeMap(),
      'title'                     => $this->title(),
      'reviewed-title'            => $this->reviewed_title(),
      'reviewed-author'           => $this->reviewed_author(),
      'publisher'                 => $this->publisher(),
      'publisher-place'           => $this->pubplace(),
      #'editor'                    => ,
      'collection-editor'         => $this->editor(),
      $authors['type']            => $authors['authors'],
      #'author'  => $authors['authors'],
      'issued'                    => $this->issued(),
      "container-title"           => $this->container(),
      'volume'                    => $this->getDataField($this->bibData, 'field_publication_volume'),
      'page'                      => $this->getDataField($this->bibData, 'field_publication_pagination'),

      'issue'                     => $this->getDataField($this->bibData, 'field_publication_issue'),
      'edition'                   => $this->edition(),


      #'medium'                      => NULL,
      #'status'                      => NULL,

      #'page-first'                  => NULL,
      #'locator'                     => NULL,
      #'version'                     => 'biblio_edition',
      #'number-of-volumes'           => 'biblio_number_of_volumes',
      # 'number-of-pages'             => NULL,


      #'chapter-number'              => 'biblio_section',











      #    'original-publisher'          => 'original-publisher' ,
      #'original-publisher-place'    => "original-publisher-place",
      #   #'section'                     => 'biblio_section',
      #'archive'                     => NULL,
      #'archive-place'               => NULL,
      #'authority'                   => NULL,
      #'archive_location'            => NULL,
      # #'genre'                       => NULL,
      #'note'                        => 'biblio_notes',
      #'annote'                      => NULL,
      #'abstract'                    => 'biblio_abst_e',
      #'keyword'                     => 'biblio_keywords',
      #'number'                      => 'biblio_number',
      # 'references'                  => NULL,
      # 'URL'                         => 'biblio_url',
      #'DOI'                         => 'biblio_doi',
      #'ISBN'                        => 'biblio_isbn',
      #'call-number'                 => 'biblio_call_number',
      #'citation-number'             => NULL,
      #'citation-label'              => 'biblio_citekey',
      #'first-reference-note-number' => NULL,
      #'year-suffix'                 => NULL,
      #'jurisdiction'                => NULL,

      //Date Variables'


      #  'accessed'                    => 'biblio_access_date',
      #  'container'                   => 'biblio_date',
      # 'original-date'               => 'biblio_date',

      //Name Variables'



      #  'translator'                  => 'biblio_contributors:3',
      #  'recipient'                   => NULL,
      #  'interviewer'                 => 'biblio_contributors:1',
      #   'composer'                    => 'biblio_contributors:1',
      #   'original-author'             => NULL,
      #   'container-author'            => NULL,
      #   'collection-editor'           => NULL,
    ];
    $this->map = json_encode($map);

  }

    private function getRanges( $aNumbers ) {
      $aNumbers = array_unique($aNumbers);
      sort($aNumbers);
      $aGroups = [];
      for ($i = 0; $i < count($aNumbers); $i++) {
        if ($i > 0 && ($aNumbers[$i - 1] == $aNumbers[$i] - 1)) {
          array_push($aGroups[count($aGroups) - 1], $aNumbers[$i]);
        }
        else {
          array_push($aGroups, [$aNumbers[$i]]);
        }
      }
      $aRanges = [];
      foreach ($aGroups as $aGroup) {
        if (count($aGroup) == 1) {
          $aRanges[] = $aGroup[0];
        }
        else {
          $aRanges[] = $aGroup[0] . '&#8211;' . $aGroup[count($aGroup) - 1];
        }
      }
      return $aRanges;
    }


}
