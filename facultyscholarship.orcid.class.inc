<?php


use GuzzleHttp\Client;
use GuzzleHttp\Exception\RequestException;
use GuzzleHttp\Exception\ClientException;

use Seboettg\CiteProc\StyleSheet;
use Seboettg\CiteProc\CiteProc;

class orcidAPI
{
  private $endpoint;
  private $sandbox;
  private $orcid;
  protected $token;
  public $id;
  private $author;
  private $verify;


  function __construct($data){
    $this->sandbox = FALSE;
    $this->server = variable_get('orcid_api_endpoint');
    $this->sandbox_server = variable_get('orcid_api_sandbox_endpoint');
    $this->orcid = $data->orcid_id;
    $this->token = 'Bearer ' . $data->orcid_token;
    $this->id = $data->user_id;
    $this->verify = false;
  }

  public function getWorks(){
    $response = $this->request('works');
    $works = [];
    foreach($response->group as $work){
      $data = $work->{'work-summary'}[0];
      $works[$data->{'put-code'}] = $data->title->title->value;
    }
    return $works;
  }
/*
$connector = new FacultyScholarshipORCID();
$data = $connector->getUvaData('mgc3r');
$orcid = new orcidAPI($data);
dsm($orcid->getWorks());
 */


  public function getWork($putcode){
    $endpoint = 'work/' . $putcode;
    return $this->request($endpoint);
  }

  public function establishEmployment(){
    $this->loadAuthor();
    if(isset($this->author->field_faculty_orcid_employment[LANGUAGE_NONE][0])) {
      $localEmploymentPutcode = $this->author->field_faculty_orcid_employment[LANGUAGE_NONE][0]['value'];
    } else {
      $localEmploymentPutcode = FALSE;
    }
    $works = $this->getEmployment();
    $confirmed = FALSE;
    foreach($works->{'affiliation-group'} as $work) {
      $source = $work->summaries[0]->{'employment-summary'}->source->{'source-name'}->value;
      $orgid = $work->summaries[0]->{'employment-summary'}->organization->{'disambiguated-organization'}->{'disambiguated-organization-identifier'};
      if($source == 'University of Virginia' && $orgid == 33740) {
        $remoteEmploymentPutcode = $work->summaries[0]->{'employment-summary'}->{'put-code'};
        if($localEmploymentPutcode != $remoteEmploymentPutcode) {
          $this->author->field_faculty_orcid_employment[LANGUAGE_NONE][0]['value'] = $remoteEmploymentPutcode;
          node_save($this->author);
        }
        return TRUE;
      }
    }
    return FALSE;
  }

  public function syncEmployment(){
    $status = $this->establishEmployment();
    if($status == FALSE){
      $this->putEmployment();
    }
  }

  public function putEmployment(){
    $endpoint = 'employment';
    $data = $this->formatEmployment();
    $response_headers = $this->send($endpoint, [], $data);
    if(isset($response_headers)) {
      $this->author->field_faculty_orcid_employment[LANGUAGE_NONE][0]['value'] =  basename($response_headers['headers']['Location'][0]);
      node_save($this->author);
    }
  }

  public function getEmployment(){
    $this->loadAuthor();
    $endpoint = 'employments';
    return $this->request($endpoint);
  }

  private function loadAuthor(){
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', 'faculty')
      ->propertyCondition('status', NODE_PUBLISHED)
      ->fieldCondition('field_computing_id', 'value', $this->id, '=');
    $result = $query->execute();
    if (isset($result['node'])) {
      $entity = entity_load('node', array_keys($result['node']));
      $this->author = array_pop($entity);
      return TRUE;
    } else {
      return FALSE;
    }
  }

  private function formatEmployment() {
    $employment = [];
    $employment["organization"] = [
      'name' => 'University of Virginia School of Law',
      'address' => [
        'city' => 'Charlottesville',
        'region' => 'VA',
        'country' => 'US'
      ],
      'disambiguated-organization' => [
        'disambiguated-organization-identifier' => 33740,
        'disambiguation-source' => 'RINGGOLD'
      ]
    ];
    $employment['url'] = 'https://law.virginia.edu/node/' . $this->author->nid;
    $employment['visibility'] = 'public';
    return json_encode($employment);
  }

  public function putCatalog(){
    $this->loadAuthor();
  }

  /**
   * fully loaded node
   *  FORCE true forces update even if node doesnt need updating, for testing or bulk sync
   * added coauths generated in class, recursive
   * @param $publication
   * @param bool $force
   * @param array $added_coauths
   * @return array
   * @throws Exception
   */

  public function putWork($publication, $force = FALSE, $added_coauths = []){
    $endpoint = 'work';
    if(isset($publication->field_publication_orcid_putcode[LANGUAGE_NONE])){
      $putcode = $publication->field_publication_orcid_putcode[LANGUAGE_NONE][0]['value'];
    }
    $formatted = $this->formatWork($publication);
    if (!isset($putcode) || $putcode == '') {
      $response = $this->send($endpoint, [], $formatted);
      if (isset($response['body']->{'response-code'})) {
        if ($response['body']->{'response-code'} == 409) {
          preg_match('/[0-9]{4,}/', $response['body']->{'developer-message'}, $matches);
          $putcode = $matches[0];
          $work = $this->getWork($putcode);
          $publication->field_publication_orcid_putcode[LANGUAGE_NONE][0]['value'] = $putcode;
          $last_modified_remote = round($work->{'last-modified-date'}->value / 1000);
          if ($publication->changed > $last_modified_remote) {
            $endpoint .= '/' . $putcode;
            $formatted = $this->formatWork($publication);
            $response = $this->update($endpoint, [], $formatted);
            $work = $this->getWork($putcode);
            $modified_timestamp = round($work->{'last-modified-date'}->value / 1000);
            $publication->field_publication_orcid_modified[LANGUAGE_NONE][0]['value'] = $modified_timestamp;
            node_save($publication);
            $publication->changed = $modified_timestamp;
            drupal_write_record('node', $modified_timestamp, 'nid');
          }
        }
        if(is_array($response['body'])) {
          if ($response['body']->{'response-code'} == 200) {
            $putcode = basename($response['headers']['Location'][0]);
            $work = $this->getWork($putcode);
            $publication->field_publication_orcid_putcode[LANGUAGE_NONE][0]['value'] = $putcode;
            $modified_timestamp = round($work->{'last-modified-date'}->value / 1000);
            $publication->field_publication_orcid_modified[LANGUAGE_NONE][0]['value'] = $modified_timestamp;
            node_save($publication);
            $publication->changed = $modified_timestamp;
            drupal_write_record('node', $modified_timestamp, 'nid');
          }
        }
      }
    }
    if (isset($putcode) && $putcode != '') {

      $work = $this->getWork($putcode);
      if (isset($work->{'response-code'})) {
        if ($work->{'response-code'} == 404) {
          unset($publication->field_publication_orcid_putcode);
          node_save($publication);
          $this->putWork($publication);
        }
      } else {

        $last_modified_remote = round($work->{'last-modified-date'}->value / 1000);
        if ($publication->changed > $last_modified_remote || $force == TRUE) {
          $endpoint .= '/' . $putcode;
          $formatted = $this->formatWork($publication);
          $response = $this->update($endpoint, [], $formatted);
          if(isset($response->{'developer-message'})) {
            $message = $response->{'developer-message'};
            drupal_set_message($publication->title . ' | ' . $message, 'warning');
            unset($publication->field_publication_orcid_modified[LANGUAGE_NONE][0]);
            $publication->field_publication_orcid_issue[LANGUAGE_NONE][0]['value'] = $message;
            node_save($publication);
          } else {
            $work = $this->getWork($putcode);
            $modified_timestamp = round($work->{'last-modified-date'}->value / 1000);
            $publication->field_publication_orcid_modified[LANGUAGE_NONE][0]['value'] = $modified_timestamp;
            unset($publication->field_publication_orcid_issue[LANGUAGE_NONE]);
            node_save($publication);
            $publication->changed = $modified_timestamp;
            drupal_write_record('node', $publication, 'nid');
          }
        }
      }
    }
    $added_coauths[] = $this->id;
    foreach ($publication->field_publication_author[LANGUAGE_NONE] as $key => $paragraph) {
      $author_entity = entity_load('paragraphs_item', [$paragraph['value']]);
      $author = array_pop($author_entity);
      $authordata = node_load($author->field_publication_author_name[LANGUAGE_NONE][0]['target_id']);
      $cid = $authordata->field_computing_id[LANGUAGE_NONE][0]['value'];
      if ($authordata->field_faculty_orcid_connector[LANGUAGE_NONE][0]['value'] == 1 && $authordata == 1) {
        $coauthors[] = $cid;
      }
    }
    $remaining = array_diff_key($coauthors, $added_coauths);
    if(count($remaining) > 0)  {
      $this->putWork($publication, TRUE, $remaining);
    }
    return $response;
  }



/*
 * Sample: https://github.com/ORCID/ORCID-Source/blob/master/orcid-model/src/main/resources/record_3.0/samples/write_samples/bulk-work-3.0.json
 * external identifiers https://groups.google.com/forum/#!topic/orcid-api-users/06kOtiAJkgg
 *
 *
 */

  function formatWork($publication){
    $paragraph = entity_load('paragraphs_item', [$publication->field_publication_data[LANGUAGE_NONE][0]['value']]);
    $pubdata = array_pop($paragraph);
    $type_tid = taxonomy_term_load($publication->field_publication_type[LANGUAGE_NONE][0]['tid']);
    $type = $type_tid->field_orcid_type[LANGUAGE_NONE][0]['value'];
    $date = $pubdata->field_publication_dates[LANGUAGE_NONE][0]['from'];
    if(isset($publication->field_publication_abstract[LANGUAGE_NONE])) {
      $abstract = $this->htmlToPlainText($publication->field_publication_abstract[LANGUAGE_NONE][0]['value']);
    }
    $identifiers = '';
    if(isset($pubdata->field_publication_identifier[LANGUAGE_NONE])) {
      $identifierdata = $pubdata->field_publication_identifier[LANGUAGE_NONE][0]['value'];
      $identifierparagraph = entity_load('paragraphs_item', [$identifierdata]);
      $identifiers = array_pop($identifierparagraph);
    }
    $fs = new facultyscholarship($publication->nid);
    $fs->showAuthor();

    $contributors = [];
    foreach($publication->field_publication_author[LANGUAGE_NONE] as $key => $paragraph){
      $authorparagraph = entity_load('paragraphs_item', [$paragraph['value'] ]);
      $author = array_pop($authorparagraph);
      $external = $author->field_publication_external_name[LANGUAGE_NONE][0];
      $internal = $author->field_publication_author_name[LANGUAGE_NONE][0]['target_id'];
      if(isset($internal)){
        $authorNode = node_load($internal);
        $nameParts["family"] = $authorNode->field_last_name[LANGUAGE_NONE][0]['value'];
        $nameParts["given"] = $authorNode->field_first_name[LANGUAGE_NONE][0]['value'];
        $orcid = $authorNode->field_faculty_orcid[LANGUAGE_NONE][0]['value'];
      } elseif($external){
        $nameParts["family"] = $external['family'];
        $nameParts["given"] = rtrim($external['given'] . " " .$external['middle']);
        $orcid = FALSE;
      }

      if($orcid != FALSE) {
        $contributors['contributor'][] = [
          'contributor-orcid' => [
            'uri' => 'https://orcid.org/' . $orcid,
            'path' => $orcid,
            'host' => 'orcid.org'
          ],
          'credit-name' => [
            'value' => $nameParts["given"] . " " . $nameParts["family"]
          ],
          'contributor-attributes' => [
            'contributor-role' => 'author'
          ]
        ];
      } else {
        $contributors['contributor'][] = [
          'credit-name' => [
            'value' => $nameParts["given"] . " " . $nameParts["family"]
          ],
          'contributor-attributes' => [
            'contributor-role' => 'author'
          ]
        ];
      }
    }
    $replacements =[
      "&rsquo;" => "'",
      "&lsquo;" => "'",
      "&quot;"  => '"',
      "\r\n"    => "",
      "&ldquo;" => '"',
      "&rdquo;" => '"',
      "&#39;"   => "'",
      "&nbsp;"  => " ",
      "\t"      => " "
    ];

    $title = strip_tags($publication->field_publication_title[LANGUAGE_NONE][0]['value']);
    foreach ($replacements as $target => $replacement) {
      $title = str_replace($target, $replacement, $title);
      if(isset($abstract)) $abstract = str_replace($target, $replacement, $abstract);
    }

    $work['title']['title']['value'] = $title;
    if(isset($publication->field_publication_orcid_putcode[LANGUAGE_NONE][0]['value'])){
      $work['put-code'] = $publication->field_publication_orcid_putcode[LANGUAGE_NONE][0]['value'];
    }
    $work['contributors'] = $contributors;
    $work['external-ids'] = $this->buildExtIDs($publication->nid, $identifiers);
    $publisher = taxonomy_term_load($pubdata->field_publication_publisher[LANGUAGE_NONE][0]['tid']);
    if($publisher){
         $pubTitle = $publisher->name;
    } else {
      $pubTitle = 'Forthcoming';
    }
    $work['journal-title']['value'] = $pubTitle;
    if(isset($abstract)) $work['short-description'] = substr($abstract, 0, 4999);
    $work['type'] = $type;
    if(isset($date['year'])){
      $work['publication-date']['year'] = ['value' => $date['year']];
      if(isset($date['month']) && $date['month'] > 0) {
        $work['publication-date']['month'] = ['value' => str_pad($date['month'], 2, 0,STR_PAD_LEFT)];
      }
      if(isset($date['day'])) $work['publication-date']['day'] = ['value' => str_pad($date['day'], 2, 0,STR_PAD_LEFT)];
    }
    $bluebook_citation = [
      'citation-type' => 'formatted-unspecified',
      #'citation-value' => $fs->citation()
    ];
    #$work['citation'][] = $bluebook_citation;
    $bibtex_citation = [
      'citation-type' => 'bibtex',
      'citation-value' => $this->buildBibtex($work, $publication)
    ];
    $work['citation'] = $bibtex_citation;
    $formatted['work'] = $work;

    return html_entity_decode(json_encode($work , JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES));

  }


  public function buildBibtex($formatted, $publication){
    $encoder = new BibtexEncoder($formatted, $publication);
    return $encoder->encode();
  }

  private function buildExtIDs($entity_id, $identifiers){
    $extids = [];
    $extids['external-id'][] = [
      'external-id-type' => 'other-id',
      'external-id-value' => 'uvalaw:' . $entity_id,
      'external-id-url' => ['value' => 'https://law.virginia.edu/node/' . $entity_id],
      'external-id-relationship' => 'self'
    ];

    if(isset($identifiers->field_publication_ssrn[LANGUAGE_NONE][0]['value'])){
      $ssrn_url = explode('=', $identifiers->field_publication_ssrn[LANGUAGE_NONE][0]['value']);
      if(count($ssrn_url) > 1){
        $ssrnid = $ssrn_url[1];
      } else {
        $ssrnid = $identifiers->field_publication_ssrn[LANGUAGE_NONE][0]['value'];
      }
      $extids['external-id'][] = [
        'external-id-type' => 'ssrn',
        'external-id-value' => $ssrnid,
        'external-id-url' => ['value' => 'http://papers.ssrn.com/abstract_id=' . $ssrnid],
        'external-id-relationship' => 'self'
      ];
    }
    if (isset($identifiers->field_publication_isbn)) {
      $isbn = $identifiers->field_publication_isbn[LANGUAGE_NONE][0]['isbn'];
      if (isset($isbn)) {
        $extids['external-id'][] = [
          'external-id-type' => 'isbn',
          'external-id-value' => $isbn,
          'external-id-url' => [ 'value' => 'https://www.worldcat.org/isbn/' . $isbn ],
          'external-id-relationship' => 'self'
        ];
      }
    }

    if (isset($identifiers->field_publication_hein)) {
      $hein = $identifiers->field_publication_hein[LANGUAGE_NONE][0]['value'];
      if (isset($hein)) {
        $pieces = explode('.', $hein);
        $hein_url = "https://heinonline.org/HOL/Page?handle=" . $pieces[0] . '.' . $pieces[1] . '&div=';
        if($pieces[2]){
          $hein_url .= $pieces[2];
        } else {
          $ivar = explode('&i=', $pieces[1]);
          $hein_url .= $ivar[1];
        }
        $extids['external-id'][] = [
          'external-id-type' => 'other-id',
          'external-id-value' => $hein,
          'external-id-url' => ['value' => $hein_url],
          'external-id-relationship' => 'self'
        ];
      }
    }
    return $extids;
  }

 function removeWork($putcode){
    $path = 'work/' . $putcode;
    $this->delete($path);

/*
 *
$connector = new FacultyScholarshipORCID();
$data = $connector->getUvaData('ev4a');
$orcid = new orcidAPI($data);
$orcid->removeWork(79403504);
 */
}

  private function update($path, $parameters = [], $body){

    if ($this->sandbox == FALSE){
      $base_uri = $this->server . $this->orcid . '/';
    } else {
      $base_uri = $this->sandbox_server . $this->orcid . '/';
    }
    $client = new Client(['base_uri' => $base_uri]);
    try {
      $response = $client->put($path,
        [
          'query' => $parameters,
          'verify' => $this->verify,
          'headers' => [
            'Authorization' => $this->token,
            'Content-Type' => 'application/vnd.orcid+json',
            'Accept' => 'application/orcid+json'
          ],
          'body' => $body,
        ]
      );
    return [
      'headers' => $response->getHeaders(),
      'body' => json_decode($response->getBody()->getContents())
    ];

    } catch (ClientException $e){
      return json_decode($e->getResponse()->getBody()->getContents());
    } catch (RequestException $e){
      echo($e->getResponse()->getBody());
    }
  }

  private function send($path, $parameters = [], $body){

    if ($this->sandbox == FALSE){
      $base_uri = $this->server . $this->orcid . '/';
    } else {
      $base_uri = $this->sandbox_server . $this->orcid . '/';
    }
    $client = new Client(['base_uri' => $base_uri,
      'verify' => $this->verify]);
    try {
      $response = $client->post($path,
        [
          'query' => $parameters,
          'headers' => [
            'Authorization' => $this->token,
            'Content-Type' => 'application/vnd.orcid+json',
            'Accept' => 'application/orcid+json'
          ],
          'body' => $body,
        ]
      );
      return [
        'headers' => $response->getHeaders(),
        'body' => json_decode($response->getBody()->getContents())
      ];
    } catch (ClientException $e){
      return ['body' => json_decode($e->getResponse()->getBody())];
    } catch (RequestException $e){

      echo($e->getResponse()->getBody());
    }
  }

  public function delete($path, $parameters = []){
    if ($this->sandbox == FALSE){
      $base_uri = $this->server . $this->orcid . '/';
    } else {
      $base_uri = $this->sandbox_server . $this->orcid . '/';
    }
    $client = new Client(['base_uri' => $base_uri,
      'verify' => $this->verify]);
    try {
      $response = $client->request('DELETE', $path,
        [
          #'http_errors' => false,
          'query' => $parameters,
          'headers' => [
            'Authorization' => $this->token,
            'Accept' => 'application/orcid+json'
          ]
        ]
      );
      #dsm( $response->getBody()->getContents()); // debug for json formatting
      return json_decode($response->getBody()->getContents());
    } catch (ClientException $e){
      return json_decode($e->getResponse()->getBody());
    } catch (RequestException $e){
      echo($e->getResponse()->getBody());
    }
  }


  public function request( $path, $parameters = []) {
    if ($this->sandbox == FALSE){
      $base_uri = $this->server . $this->orcid . '/';
    } else {
      $base_uri = $this->sandbox_server . $this->orcid . '/';
    }
    $client = new Client(['base_uri' => $base_uri,
      'verify' => $this->verify]);
    try {
      $response = $client->request('GET', $path,
        [
          'http_errors' => false,
          'query' => $parameters,
          'headers' => [
            'Authorization' => $this->token,
            'Accept' => 'application/orcid+json'
          ]
        ]
      );
      #dsm( $response->getBody()->getContents()); // debug for json formatting
      return json_decode($response->getBody()->getContents());
    } catch (ClientException $e){
        return json_decode($e->getResponse()->getBody());
    } catch (RequestException $e){
        echo($e->getResponse()->getBody());
    }
  }

  private function htmlToPlainText($str){
    $str = str_replace('&nbsp;', ' ', $str);
    $str = str_replace('&mdash;', '--', $str);
    $str = str_replace('&rsquo;', "'", $str);
    $str = str_replace("\n", '', $str);
    $str = str_replace("\r", '', $str);
    $str = strip_tags($str);

    return $str;
  }
}

