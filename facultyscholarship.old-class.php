<?php

use Seboettg\CiteProc\StyleSheet;
use Seboettg\CiteProc\CiteProc;
use Sebeottg\CiteProc\Rendering;





class FacultyPublication {
    var $nid;
    private $data;
    var $style;
    var $field_label;

    public function __construct($entity)
    {
        $this->entity = $entity;
        $this->publication = entity_metadata_wrapper('node', $this->entity);
        $this->style = 'chicago-fullnote-bibliography';
        #$this->bibData = $this->processParagraph('field_publication_data');
        #$this->authorData = $this->processParagraph('field_publication_author');
        $this->processType();
        $this->processTitle();
        $this->processAuthors();
        $this->prepareBibData();
        $this->processPublisher();
        $this->processFields();


    }

    function processType(){
        $type = $this->publication->field_publication_type->value();

        switch($type->name){
            case "Textbook":
                $this->data->type = "book";
                break;
            default:
                $this->data->type = strtolower($type->name);
                break;
        }
    }

    function report(){
        $publication = $this->publication;
        if ($publication->getBundle() == PUBLICATION_ENTITY) {
            dsm($publication->getPropertyInfo());

        }
    }

    function processAuthors(){
        foreach( $this->publication->field_publication_author->value() as $authorData){
            $authorParagraph = entity_metadata_wrapper('paragraphs_item', $authorData->item_id);
            $authorRole = $authorParagraph->field_publication_author_role->value();
            $authorNode = entity_metadata_wrapper('node', $authorParagraph->field_publication_author_name->value());
            unset($nameParts);
            if($authorNode){
              $nameParts->family = $authorNode->field_last_name->value();
              $nameParts->given = $authorNode->field_first_name->value();
              $nameParts->role = $authorRole;
            }

            $author[] = $nameParts;
        }
        $this->data->author = $author;
    }
    function prepareBibData(){
        $bibData = $this->publication->field_publication_data->value();
        $this->bibData = entity_metadata_wrapper('paragraphs_item', $bibData[0]->item_id); //array_shift
        if ( count($bibData > 1)){
            foreach ($bibData as $edition){
                $date = $edition->field_publication_date[LANGUAGE_NONE][0]['from']['year'];
                $edition = entity_metadata_wrapper("paragraphs_item", $edition->item_id);
                $number = $edition->field_publication_edition->value();
                $ordinal = $this->ordinal($number);
                $text = $ordinal . " ed. " . $date;
                $editions[] = [
                    'number'    => (int) $number,
                    'date'      => $date,
                    'ordinal'   => $ordinal,
                    'text'      => $text
                ];
            }
            $this->editions = $editions;
        }
    }
    function processParagraph($field_label = NULL){
        $paragraphWrapper = $this->publication->{$field_label}->value();
        return $paragraphWrapper[0];
    }
    function processParagraphs($field_label = NULL){
        $paragraphWrapper = $this->publication->{$field_label}->value();
        return $paragraphWrapper;
    }

    function processFieldMap() {
        $publication = $this->publication;

        $title = $publication->field_publication_title->value();
        $cslFieldMap = ['title' => $title['value']];

        dsm($cslFieldMap);

    }

    function processTitle(){
        $title = $this->publication->field_publication_title->value();
        $this->data->title = trim($title['value']);
    }

    function processPublisher(){
        $publisherTerm = $this->bibData->field_publication_publisher->value();
        $publisherParagraph = entity_metadata_wrapper('taxonomy_term', $publisherTerm->tid);
        $this->data->publisher = $publisherTerm->name;
        $this->data->{'publisher-place'} = $publisherParagraph->field_publication_location->value();
    }
    function processIdentifiers(){
      #  $bibDataParagraph = entity_metadata_wrapper('paragraphs_item', $bibData[0]->item_id);


    }

    function processFields(){

        $bibData = array_shift($this->publication->field_publication_data->value());

        switch ($this->data->type){
            case "book":
                $year = $bibData->field_publication_date[LANGUAGE_NONE][0]['from']['year'];
                $this->data->issued->{'date-parts'}[][] = $year;
                break;
            default:


                break;
        }




        #dsm($bibDataParagraph->field_publication_date->value());




        #$bibdata = $this->processParagraph('field_publication_data');

        #dsm($bibdata);





    }

    function processDate($type, $date){



    }
    function pubID(){
        $publication->id = $this->data->id;
        return [$publication];

    }
    function bib(){
        #$data = [$this->data];







        $this->style = "bluebook2";

        $this->style = "elsevier-harvard";

        $this->style = "harvard-north-west-university";


        $stylesheet = StyleSheet::loadStyleSheet($this->style);
        $citeProc = new CiteProc($stylesheet);






        dsm($citeProc->render([$this->data], "bibliography"));

    }

    function citeproc_style($variables){
        static $citeproc;
        $node = $variables['node'];
        $style = isset($variables['style_name']) ? $variables['style_name'] : NULL;

       # module_load_include('inc', 'facultyscholarship', 'CSL');



    }

    function processOlsonBib(){

        switch($this->data->type){
            case "book":
                $publication[] = "<i>" . $this->data->title ."</i>";
                if (count ($this->data->author) > 1) {
                    unset($this->data->author[array_key_first($this->data->author)]);
                        array_values($this->data->author);
                        if (count($this->data->author) == 1){
                            $authors = $this->data->author[1]->given . " " . $this->data->author[1]->family ;
                        } elseif (count($this->data->author) > 1) {
                            foreach ($this->data->author as $supplementalAuthor) {
                                $supplementalauthorAuthors[] = $supplementalAuthor->given . " " . $supplementalAuthor->family;
                            }
                            $last = array_pop($supplementalauthorAuthors);
                            array_push($supplementalauthorAuthors, 'and ' . $last);
                            if(count($supplementalauthorAuthors) > 2){
                                $authors = implode(" ",$supplementalauthorAuthors);
                            } else {
                                $authors = implode(" ",$supplementalauthorAuthors);

                            }
                        }
                        array_push($publication, "(with " . $authors . ")");

                }
                #if ($this->data->author[0]->role == 'editor') $publication[] = "(ed)";
                    if(count($this->editions) > 1){
                        $first = array_slice($this->editions, 0,1);
                        unset($this->editions[array_key_first($this->editions)]);
                        foreach($this->editions as $edition){
                            $editions[] = $edition['text'];
                        }
                        $editions_output .= $first[0]['date'] . "; " . implode("; ", $editions);
                } elseif (count($this->editions) == 1  && $this->editions[0]['number'] > 1){
                    $editions_output = $this->editions[0]['text'];
                } else {
                    $editions_output = $this->editions[0]['date'];
                }

                if (isset($this->data->publisher)) $publication_output = $this->data->publisher;
                if (isset($this->data->publisher) && isset($editions_output)) $publication_output .= ", ";
                if (isset($editions_output)) $publication_output .= $editions_output;
                if (isset($publication_output)) $publication_output = "(" . $publication_output . ")";
                array_push($publication, $publication_output);

        }
      #  dsm($publication);
        return implode(" ", $publication);

   }
   function ob_editions(){

   }
    public static function ordinal($num) {
        if (($num / 10) % 10 == 1) {
            $ordinalSuffix = "th";
        } elseif ($num % 10 == 1) {
            $ordinalSuffix = "st";
        } elseif ($num % 10 == 2) {
            $ordinalSuffix = "nd";
        } elseif ($num % 10 == 3) {
            $ordinalSuffix = "rd";
        } else {
            $ordinalSuffix = "th";
        }
        if (empty($ordinalSuffix)) {
            $ordinalSuffix = "";
        }
        return $num . $ordinalSuffix;
    }
}






/*
 * function _get_csl_field_map() {
  $map['field_map'] =  serialize(
        array(
          'title'                       => ['field_publication_title', 'long_text']                           ******
          'publisher'                   => 'biblio_publisher',                  ******
          'publisher-place'             => 'biblio_place_published',            ******
          'original-publisher'          => '',
          'original-publisher-place'    => '',
          'archive'                     => '',
          'archive-place'               => '',
          'authority'                   => '',
          'archive_location'            => '',
          'event'                       => 'biblio_secondary_title',
          'event-place'                 => 'biblio_place_published',
          'page'                        => 'biblio_pages',                      ******
          'page-first'                  => '',
          'locator'                     => '',
          'version'                     => 'biblio_edition',                    ******
          'volume'                      => 'biblio_volume',                     ******
          'number-of-volumes'           => 'biblio_number_of_volumes',          ******
          'number-of-pages'             => '',
          'issue'                       => 'biblio_issue',                      ******
          'chapter-number'              => 'biblio_section',
          'medium'                      => '',
          'status'                      => '',
          'edition'                     => 'biblio_edition',                    ******
          'section'                     => 'biblio_section',
          'genre'                       => '',
          'note'                        => 'biblio_notes',
          'annote'                      => '',
          'abstract'                    => 'biblio_abst_e',                     ******
          'keyword'                     => 'biblio_keywords',
          'number'                      => 'biblio_number',
          'references'                  => '',
          'URL'                         => 'biblio_url',                        ******
          'DOI'                         => 'biblio_doi',                        ******
          'ISBN'                        => 'biblio_isbn',                       ******
          'call-number'                 => 'biblio_call_number',
          'citation-number'             => '',
          'citation-label'              => 'biblio_citekey',
          'first-reference-note-number' => '',
          'year-suffix'                 => '',
          'jurisdiction'                => '',

        //Date Variables'

          'issued'                      => 'biblio_year',
          'event'                       => 'biblio_date',
          'accessed'                    => 'biblio_access_date',
          'container'                   => 'biblio_date',
          'original-date'               => 'biblio_date',

        //Name Variables'

          'author'                      => 'biblio_contributors:1',             ******
          'editor'                      => 'biblio_contributors:2',             ******
          'translator'                  => 'biblio_contributors:3',
          'recipient'                   => '',
          'interviewer'                 => 'biblio_contributors:1',
          'publisher'                   => 'biblio_publisher',
          'composer'                    => 'biblio_contributors:1',
          'original-publisher'          => '',
          'original-author'             => '',
          'container-author'            => '',
          'collection-editor'           => '',
         )
        );
  $map['format'] = 'csl';
  return $map;
}
 */
