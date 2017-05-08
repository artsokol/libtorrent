#include "libtorrent/navigable_small_world/term_vector.hpp"
#include <algorithm>
#include <cmath>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <locale>

#include <math.h>

namespace libtorrent { namespace nsw {

size_t term_vector::levenshteinDist(const std::wstring& textString1, const std::wstring& textString2)
{
  const size_t m(textString1.size());
  const size_t n(textString2.size());

  if( m==0 ) return n;
  if( n==0 ) return m;

  size_t *costs = new size_t[n + 1];

  for( size_t k=0; k<=n; k++ ) costs[k] = k;

  size_t i = 0;
  for ( std::wstring::const_iterator it1 = textString1.begin(); it1 != textString1.end(); ++it1, ++i )
  {
    costs[0] = i+1;
    size_t corner = i;

    size_t j = 0;
    for ( std::wstring::const_iterator it2 = textString2.begin(); it2 != textString2.end(); ++it2, ++j )
    {
      size_t upper = costs[j+1];
      if( *it1 == *it2 )
      {
          costs[j+1] = corner;
      }
      else
      {
        size_t t(upper<corner?upper:corner);
        costs[j+1] = (costs[j]<t?costs[j]:t)+1;
      }

      corner = upper;
    }
  }

  size_t result = costs[n];
  delete [] costs;

  return result;
}

size_t term_vector::levenshteinDist(const std::string& textString1, const std::string& textString2)
{
  const size_t m(textString1.size());
  const size_t n(textString2.size());

  if( m==0 ) return n;
  if( n==0 ) return m;

  size_t *costs = new size_t[n + 1];

  for( size_t k=0; k<=n; k++ ) costs[k] = k;

  size_t i = 0;
  for ( std::string::const_iterator it1 = textString1.begin(); it1 != textString1.end(); ++it1, ++i )
  {
    costs[0] = i+1;
    size_t corner = i;

    size_t j = 0;
    for ( std::string::const_iterator it2 = textString2.begin(); it2 != textString2.end(); ++it2, ++j )
    {
      size_t upper = costs[j+1];
      if( *it1 == *it2 )
      {
          costs[j+1] = corner;
      }
      else
      {
        size_t t(upper<corner?upper:corner);
        costs[j+1] = (costs[j]<t?costs[j]:t)+1;
      }

      corner = upper;
    }
  }

  size_t result = costs[n];
  delete [] costs;

  return result;
}

void term_vector::alignVectors(wvector_t& textVec1, wvector_t& textVec2)
{
    wvector_t toAddFor1;
    wvector_t toAddFor2;

    for(wvector_t::iterator it1 = textVec1.begin(); it1 != textVec1.end(); ++it1)
    {
        wvector_t::iterator it2 = textVec2.find(it1->first);
        if(it2 == textVec2.end())
        {
            toAddFor2[it1->first] = ZERO_FREQUENCY_CONST;
        }
    }

    for(wvector_t::iterator it2 = textVec2.begin(); it2 != textVec2.end(); ++it2)
    {
        wvector_t::iterator it1 = textVec1.find(it2->first);
        if(it1 == textVec1.end())
        {
            toAddFor1[it2->first] = ZERO_FREQUENCY_CONST;
        }
    }

    textVec1.insert(toAddFor1.begin(),toAddFor1.end());
    textVec2.insert(toAddFor2.begin(),toAddFor2.end());
}

void term_vector::alignVectors(vector_t& textVec1, vector_t& textVec2)
{
    vector_t toAddFor1;
    vector_t toAddFor2;

    for(vector_t::iterator it1 = textVec1.begin(); it1 != textVec1.end(); ++it1)
    {
        vector_t::iterator it2 = textVec2.find(it1->first);
        if(it2 == textVec2.end())
        {
            toAddFor2[it1->first] = ZERO_FREQUENCY_CONST;
        }
    }

    for(vector_t::iterator it2 = textVec2.begin(); it2 != textVec2.end(); ++it2)
    {
        vector_t::iterator it1 = textVec1.find(it2->first);
        if(it1 == textVec1.end())
        {
            toAddFor1[it2->first] = ZERO_FREQUENCY_CONST;
        }
    }

    textVec1.insert(toAddFor1.begin(),toAddFor1.end());
    textVec2.insert(toAddFor2.begin(),toAddFor2.end());
}

double term_vector::hardCosineMeasure(const wvector_t& textVec1, const wvector_t& textVec2)
{
    wvector_t textVec1Copy(textVec1);
    wvector_t textVec2Copy(textVec2);

    alignVectors(textVec1Copy,textVec2Copy);

    TORRENT_ASSERT(textVec1Copy.size() == textVec2Copy.size());

    double dot = 0.0, denom1 = 0.0, denom2 = 0.0;

    for(wvector_t::iterator it1 = textVec1Copy.begin(); it1 != textVec1Copy.end(); ++it1)
    {
        wvector_t::iterator it2 = textVec2Copy.find(it1->first);

        //double wordDist = levenshteinDist(it1->first,it2->first);

        dot += it1->second * it2->second;// * wordDist;

        denom1 += it1->second * it1->second;
        denom2 += it2->second * it2->second;

    }

    //cos(O) = a*b/(||a||*||b||)
    return dot / (sqrt(denom1) * sqrt(denom2)) ;
}

double term_vector::hardCosineMeasure(const vector_t& textVec1, const vector_t& textVec2)
{
    vector_t textVec1Copy(textVec1);
    vector_t textVec2Copy(textVec2);

    alignVectors(textVec1Copy,textVec2Copy);

    TORRENT_ASSERT(textVec1Copy.size() == textVec2Copy.size());

    double dot = 0.0, denom1 = 0.0, denom2 = 0.0;

    for(vector_t::iterator it1 = textVec1Copy.begin(); it1 != textVec1Copy.end(); ++it1)
    {
        vector_t::iterator it2 = textVec2Copy.find(it1->first);

       // double wordDist = levenshteinDist(it1->first,it2->first);

        dot += it1->second * it2->second;// * wordDist;

        denom1 += it1->second * it1->second;
        denom2 += it2->second * it2->second;

    }

    //cos(O) = a*b/(||a||*||b||)
    return dot / (sqrt(denom1) * sqrt(denom2)) ;
}

double term_vector::softCosineMeasure(const wvector_t& textVec1, const wvector_t& textVec2)
{
    wvector_t textVec1Copy(textVec1);
    wvector_t textVec2Copy(textVec2);

    alignVectors(textVec1Copy,textVec2Copy);

    TORRENT_ASSERT(textVec1Copy.size() == textVec2Copy.size());

    double dot = 0.0, denom1 = 0.0, denom2 = 0.0;

    for(wvector_t::iterator it1 = textVec1Copy.begin(); it1 != textVec1Copy.end(); ++it1)
    {
        for(wvector_t::iterator it2 = textVec2Copy.begin(); it2 != textVec2Copy.end(); ++it2)
        {
            double wordDist = 1/(1+levenshteinDist(it1->first,it2->first));
            dot += it1->second * it2->second * wordDist;
        }

        wvector_t::iterator it11 = it1;
        for(++it1; it11 != textVec1Copy.end(); ++it11)
        {
            double wordDist = 1/(1+levenshteinDist(it1->first,it11->first));
            denom1 += it1->second * it11->second * wordDist;
        }
    }

    for(wvector_t::iterator it2 = textVec2Copy.begin(); it2 != textVec2Copy.end(); ++it2)
    {
        wvector_t::iterator it22 = it2;
        //++it22;
        for(++it22; it22 != textVec2Copy.end(); ++it22)
        {
            double wordDist = 1/(1+levenshteinDist(it2->first,it22->first));
            denom2 += it2->second * it22->second * wordDist;
        }
    }

    return dot / (sqrt(denom1) * sqrt(denom2)) ;
}

double term_vector::softCosineMeasure(const vector_t& textVec1, const vector_t& textVec2)
{
    vector_t textVec1Copy(textVec1);
    vector_t textVec2Copy(textVec2);

    alignVectors(textVec1Copy,textVec2Copy);

    TORRENT_ASSERT(textVec1Copy.size() == textVec2Copy.size());

    double dot = 0.0, denom1 = 0.0, denom2 = 0.0;

    for(vector_t::iterator it1 = textVec1Copy.begin(); it1 != textVec1Copy.end(); ++it1)
    {
        for(vector_t::iterator it2 = textVec2Copy.begin(); it2 != textVec2Copy.end(); ++it2)
        {
            double wordDist = 1/(1+levenshteinDist(it1->first,it2->first));
            dot += it1->second * it2->second * wordDist;
        }

        for(vector_t::iterator it11 = it1; it11 != textVec1Copy.end(); ++it11)
        {
            double wordDist = 1/(1+levenshteinDist(it1->first,it11->first));
            denom1 += it1->second * it11->second * wordDist;
        }
    }

    for(vector_t::iterator it2 = textVec2Copy.begin(); it2 != textVec2Copy.end(); ++it2)
    {
        for(vector_t::iterator it22 = it2; it22 != textVec2Copy.end(); ++it22)
        {
            double wordDist = 1/(1+levenshteinDist(it2->first,it22->first));
            denom2 += it2->second * it22->second * wordDist;
        }
    }

    return dot / (sqrt(denom1) * sqrt(denom2)) ;
}

double term_vector::getVecSimilarity(const wvector_t& textVec1, const wvector_t& textVec2)
{
    if(textVec1.empty() || textVec2.empty())
        return 0.0;
    return term_vector::hardCosineMeasure(textVec1, textVec2);
}

double term_vector::getVecSimilarity(const vector_t& textVec1, const vector_t& textVec2)
{
    if(textVec1.empty() || textVec2.empty())
        return 0.0;
    return term_vector::hardCosineMeasure(textVec1, textVec2);
}


double term_vector::getSimilarity(const std::wstring& textString1, const std::wstring& textString2)
{
    wvector_t textStringVec1;
    wvector_t textStringVec2;

    return getVecSimilarity(makeTermVector(textString1,textStringVec1),
                            makeTermVector(textString2,textStringVec2));
}

double term_vector::getSimilarity(const std::string& textString1, const std::string& textString2)
{

    vector_t textStringVec1;
    vector_t textStringVec2;

    return getVecSimilarity(makeTermVector(textString1,textStringVec1),
                            makeTermVector(textString2,textStringVec2));
}

wvector_t& term_vector::makeTermVector(const std::wstring& textString, wvector_t& termVector)
{
    using w_tokenizer = boost::tokenizer<boost::char_separator<wchar_t>,
    std::wstring::const_iterator, std::wstring>;

    uint32_t wordCount = 0;

    //clearing
    if(termVector.size()>0)
        termVector.erase(termVector.begin(),termVector.end());

    if (textString == L"")
        return termVector;

    w_tokenizer tokObj(textString);
    for (w_tokenizer::iterator it = tokObj.begin(); it != tokObj.end(); ++it)
    {
        std::wstring word((*it).begin(),(*it).end());

        //all symbols to lower case
        boost::algorithm::to_lower(word);

        //filling the container
       (termVector.find(word) != termVector.end())?
                        ++termVector[word]:
                         termVector[word] = 1.0;

        ++wordCount;
    }

    for_each(termVector.begin(),termVector.end(),[&wordCount](wvector_t::value_type& table_item)
                                                    { table_item.second /= wordCount;});

    return termVector;
}

vector_t& term_vector::makeTermVector(const std::string& textString, vector_t& termVector)
{
    uint32_t wordCount = 0;

    //clearing
    if(termVector.size()>0)
        termVector.erase(termVector.begin(),termVector.end());

    if (textString == "")
        return termVector;

    boost::tokenizer<> tokObj(textString);
    for (boost::tokenizer<>::iterator it = tokObj.begin(); it != tokObj.end(); ++it)
    {
        std::string word((*it).begin(),(*it).end());

        //all symbols to lower case
        boost::algorithm::to_lower(word);

        //filling the container
       (termVector.find(word) != termVector.end())?
                        ++termVector[word]:
                         termVector[word] = 1.0;

        ++wordCount;
    }

    for_each(termVector.begin(),termVector.end(),[&wordCount](vector_t::value_type& table_item)
                                                    { table_item.second /= wordCount;});

    return termVector;
}

std::wstring term_vector::vectorToString(const wvector_t& termVector)
{
    std::wstring out;
    for_each(termVector.begin(),termVector.end(),[&out](const std::pair<std::wstring, double>& map_item)
                                                        {
                                                            double dummy_param;
                                                            double mantissa = std::modf(map_item.second, &dummy_param);
                                                            mantissa = (mantissa == 0)?0:mantissa*10e6;

                                                            out += map_item.first + L":" + std::to_wstring(int(mantissa)) + L",";
                                                        });
    out.pop_back();
    return out;
}

std::string term_vector::vectorToString(const vector_t& termVector)
{
    std::string out;
    for_each(termVector.begin(),termVector.end(),[&out](const std::pair<std::string, double>& map_item)
                                                        {
                                                            double dummy_param;
                                                            double mantissa = std::modf(map_item.second, &dummy_param);
                                                            mantissa = (mantissa == 0)?0:mantissa*10e6;

                                                            out += map_item.first + ":" + std::to_string(int(mantissa)) + ",";
                                                        });
    out.pop_back();
    return out;
}

entry& term_vector::vectorToEntry(const vector_t& termVector, entry& a)
{
  for_each(termVector.begin(),termVector.end(),[&a](const std::pair<std::string, double>& map_item)
                                                        {
                                                            double dummy_param;
                                                            double mantissa = std::modf(map_item.second, &dummy_param);
                                                            mantissa = (mantissa == 0)?0:mantissa*10e6;

                                                            a[map_item.first] = static_cast<int>(mantissa);
                                                        });
  return a;
}

}}
