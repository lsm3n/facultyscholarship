<?php



class BibtexEncoder
{
  protected $publication;
  protected $formatted;

  function __construct($formatted, $publication){
    $this->publication = $publication;
    $this->formatted = $formatted;

  }

  function encode(){
    return $this->build();
  }

  protected function crosswalk(){
    $data = [];
    $names = [];

    foreach ($this->publication->field_publication_author[LANGUAGE_NONE] as $authorData) {
      $authorParagraph = entity_load('paragraphs_item', [$authorData['value']]);
      $author = array_pop($authorParagraph);
      $external = $author->field_publication_external_name[LANGUAGE_NONE][0];
      $internal = $author->field_publication_author_name[LANGUAGE_NONE][0]['target_id'];
      if (isset($internal)) {
        $authorNode = node_load($internal);
        $nameParts["family"] = $authorNode->field_last_name[LANGUAGE_NONE][0]['value'];
        $nameParts["given"] = $authorNode->field_first_name[LANGUAGE_NONE][0]['value'];
      } elseif ($external) {
        $nameParts["family"] = $external['family'];
        $nameParts["given"] = rtrim($external['given'] . " " . $external['middle']);
      }
      $names[] = $nameParts["given"] . " " . $nameParts["family"];

    }
    $authors = implode(" and ", $names);
    $data['author'] = $authors;
    $data['title']  = $this->formatted['title']['title']['value'];
    $data['issue_date']  = $this->formatted['publication-date']['year']['value'];
    if(isset($pubdata->field_publication_volume[LANGUAGE_NONE][0]['value'])){
      $data['volume']  = $pubdata->field_publication_volume[LANGUAGE_NONE][0]['value'];
    }
    if(isset($pubdata->field_publication_issue[LANGUAGE_NONE][0]['value'])){
      $data['number']  = $pubdata->field_publication_issue[LANGUAGE_NONE][0]['value'];
    }
    if(isset($pubdata->field_publication_pagination[LANGUAGE_NONE][0]['value'])){
      $data['pages']  = $pubdata->field_publication_pagination[LANGUAGE_NONE][0]['value'];
    }
    if(isset($this->formatted['publication-date']['month'])){
      $data['month']  = $this->formatted['publication-date']['month']['value'];
    }
    $data['year']  = $this->formatted['publication-date']['year']['value'];
    $data['url']  = 'https://law.virginia.edu/node/' . $this->publication->nid;
    $publisher = taxonomy_term_load($pubdata->field_publication_publisher[LANGUAGE_NONE][0]['tid']);
    switch($this->formatted['type']){
      case "book":
      case "chapter":
        $data['publisher']  = $publisher->name;
        break;
      default:
        $data['journal']  = $publisher->name;
        break;
    }

    return $data;
  }

  protected function build(){
    $entry = $this->prepend();
    foreach ($this->crosswalk() as $key => $value) {
      $entry .= $this->buildLine($key, $value);
    }
    $entry .= $this->end();
    return $entry;
  }

  protected function buildLine($key, $value) {
    return '  ' . $key . ' = {' . $value . '},';
  }

  protected function prepend(){
    $reference = 'uvalaw:' . $this->publication->nid;
    $type = $this->formatted['type'];
    return '@' . $type . '{' . $reference . ',';
  }

  protected function end(){
    return "}";
  }

}

