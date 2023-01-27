<?php

use GuzzleHttp\Client;

class FacultyScholarshipORCID
{
    protected $token;
    private $server;
    private $sandbox;
    private $sandbox_server;
    protected $sandbox_token;
    private $roles;

    function __construct(){
      $this->token = variable_get('uva_orcid_api_token');
      $this->server  = variable_get('uva_orcid_prod_server');
      $this->sandbox = FALSE;
      $this->sandbox_server = variable_get('uva_orcid_sandbox_server');
      $this->sandbox_token = variable_get('uva_orcid_sandbox_token');
      $this->roles = [6486, 6471];
    }

    function beach(){
      /*
      $connector = new FacultyScholarshipORCID();
      $data = $connector->getUvaData('rjb6f');
      $orcid = new orcidAPI($data);
#dsm($orcid->getWorks());
      dsm($orcid->getWork(35692539));
      dsm($orcid->putWork(node_load(798921), TRUE));
*/
      $path = 'api/users/pv6f';  //pierre

      $path = 'api/users/lsm3n'; // loren
      $data = $this->request($path);
      $orcid_api = new orcidAPI($data);
      $pub = node_load(790346);
      $formatted = $orcid_api->putWork($pub, TRUE);

      #$data = $orcid_api->buildBibtex($formatted , $pub);


      return $data;

    }

    public function syncRegistered(){
      $fsa = new FacultyScholarshipAdmin();
      $list = $fsa->faculty(FALSE);
      foreach($list as $faculty){
        if(isset($faculty['field_computing_id_value'])) {
          $data = $this->getUvaData($faculty['field_computing_id_value']);
          if (isset($data->orcid_token)) {
            $users[] = $data;
            $this->localOrcidCheck($data, $faculty);
          }
        }
      }
      return $users;
    }



    private function localOrcidCheck($data, $faculty){
      if ($data->orcid_id != NULL){
        $member = node_load($faculty['nid']);
        if (!isset($member->field_faculty_orcid_id[LANGUAGE_NONE][0]['value'])){
          $member->field_faculty_orcid[LANGUAGE_NONE][0]['value'] = $data->orcid_id;
        }
        $member->field_faculty_orcid_connector[LANGUAGE_NONE][0]['value'] = 1;
        node_save($member);
        $orcid_api = new orcidAPI($data);
        #$orcid_api->establishEmployment();
      } else {
        #unregistered with uva orcid connector
      }
    }

    function getUvaData($fid){
      return $this->queryConnector($fid);
    }

    function queryConnector($fid){
      $path = 'api/users/' . $fid;
      return $this->request($path, []);
    }

    public function request( $path, $parameters = []) {
      if ($this->sandbox == FALSE){
        $client = new Client(
          [
            'base_uri' => $this->server,
            'verify' => false
          ]
        );
        $token = $this->token;
      } else {
        $client = new Client(['base_uri' => $this->sandbox_server]);
        $token = $this->sandbox_token;
      }
      try{
        $response = $client->request('GET',
          $path,
          [
            'query' => $parameters,
            'headers' => [
              'Authorization' => $token,
            ],
          ]
        );
        return json_decode($response->getBody()->getContents());
      } catch (ClientException $e){
        echo($e->getResponse()->getBody());
      } catch (RequestException $e){
        echo($e->getResponse()->getBody());
      } catch (BadException $e){
        echo $e->getCode();
      }

    }


}

