<?php




class FacultyScholarshipSSRN
{
    const SSRN_AUTH_PUBS = 'https://papers.ssrn.com/sol3/cf_dev/AbsByAuth.cfm?per_id=';
    const SSRN_AUTH_ABS = 'https://ssrn.com/abstract=';
    function __construct()
    {
      require_once ('simplehtmldom/simple_html_dom.php');

    }

    function listPublications($fsid)
    {
      $profile = node_load($fsid);
      $ssrn_id = $profile->field_faculty_id_ssrn[LANGUAGE_NONE][0]['value'];
      $html_response = file_get_html(self::SSRN_AUTH_PUBS . $ssrn_id);
      $publications = [];
      $stats = $html_response->find('div.stats-col');
      $statscolumns = $stats[0]->find('h1');
      $pubcount = strip_tags($statscolumns[0]);

      foreach($html_response->find('a.title') as $title) {
        $link = $title->href;

        if($link != 'javascript:void(0);') {
          $parts = explode('=', $link);
          $publications[] = $parts[1];
        }
      }
      return [
        'count' => $pubcount,
        'results' => $publications
      ];
    }

    function retrievePublications($fsid){
      $fsid = 117366;
      $publications = $this->listPublications($fsid);
      $match_keywords = "Keywords:";
      $match_date = "Date Written: ";
      $results = [];
      foreach ($publications['results'] as $result) {
        unset($title); unset($abstract); unset($keywords); unset($date);


        $url = self::SSRN_AUTH_ABS . $result;
        $html_response = file_get_html($url);
        $data = $html_response->find('div.box-abstract-main');
        $title_find = $data[0]->find('h1');
        $title = strip_tags($title_find[0], '<em><strong>');
        $abstract_find = $data[0]->find('div.abstract-text');
        $abstract = implode('', $abstract_find[0]->find('p'));
        foreach ($data[0]->find('p') as $item){
          if(preg_match("/{$match_keywords}/i", $item)){
            $keyword_items = str_replace($match_keywords, "", $item);
            $keywords = explode(", ", strip_tags($keyword_items));
          }
          if(preg_match("/{$match_date}/i", $item)){
            $date  = strip_tags(str_replace($match_date, "", $item));

          }
        }

        $parsed = [
          'title' => $title,
          'abstract' => $abstract,
          'keywords' => $keywords,
          'date_written' => $date
        ];



        $results[$result] = $parsed;
      }

      dsm($results);


    }

    /*
      $ssrn = new FacultyScholarshipSSRN();
$ssrn->retrievePublications(211);
    */


    function ret(){
      $fsid = 117441;
      $profile = node_load($fsid);
      $ssrn_id = $profile->field_faculty_id_ssrn[LANGUAGE_NONE][0]['value'];
      $html_response = file_get_html(self::SSRN_AUTH_PUBS . $ssrn_id);
      $publications = [];
      foreach($html_response->find('div.abs') as $title) {
        if($title->id) $publications[] = $title->id;
      }
dsm($publications);

    }



}

