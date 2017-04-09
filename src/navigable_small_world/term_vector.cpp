#include "libtorrent/navigable_small_world/term_vector.hpp"
#include <algorithm>
#include <cmath>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <locale>

using namespace std;
using namespace boost::unordered;
namespace libtorrent {
namespace nsw
{

// int term_vector::parseFile(const char* textFile)
// {
//     if(!textFile)
//         return -1;

//     std::ifstream fs;
//     std::string line;
//     unsigned long wordCountInFile = 0;

//     fs.open (textFile, std::ifstream::in );

//     if (!fs.is_open())
//         return -1;

//     while(std::getline(fs,line))
//     {
//         wordCountInFile += string2vec(line);

//     }

//     for (boost::unordered::unordered_map<std::wstring, double>::iterator it = m_tf.begin(); it != m_tf.end(); ++it)
//     {
//         it->second /= wordCountInFile;
//         // cout << " key: " << std::get<0>(hashPair) << " |value: " << std::get<1>(hashPair) << endl;
//     }

//   fs.close();
//   return 0;
// }

// unsigned int term_vector::string2vec(const std::string& line)
// {
//     unsigned long wordCount = 0;
//     //cleaning
//     boost::tokenizer<> tokObj(line);
// //    m_observer->nsw_log(nsw_logger::node,
// //                "Line %s",line.c_str());
//     for (boost::tokenizer<>::iterator it = tokObj.begin(); it != tokObj.end(); ++it)
//     {

//         //all symbols to lower case
//         wstring aWord((*it).begin(),(*it).end()); //string to wstring

//         //todo: correct forlatin symbols only
//         boost::algorithm::to_lower(aWord);

//         //filling the container
//         boost::unordered::unordered_map<std::wstring, double>::iterator isAlreadyExisted = m_tf.find(aWord);

//         (isAlreadyExisted == m_tf.end()) ?
//                             m_tf[aWord] = 1 :
//                             ++isAlreadyExisted->second;


//         ++wordCount;
//     }
//     return wordCount;
// }

// //int term_vector::parseWString(const std::wstring& line)
// //{
// //}
// //calculate a similarity between two text vectors (theoretical vectors implemented as hash tables).
// double term_vector::cosineSimilarity(const boost::unordered_map<std::wstring, double>& tf_idf_weights_hash1, const boost::unordered_map<std::wstring, double>& tf_idf_weights_hash2)
// {
//     if(tf_idf_weights_hash1.size() != tf_idf_weights_hash2.size())
//     {
//         //if (m_observer) m_observer->nsw_log(nsw_logger::node,
//         //    "Error: sizes of containers have not equal sizes. Similarity could not be calculated!");
//         return 0.0;
//     }

//     double dot = 0.0, denom1 = 0.0, denom2 = 0.0;


//     for(unordered_map<std::wstring, double>::const_iterator hash1_it = tf_idf_weights_hash1.begin(); hash1_it != tf_idf_weights_hash1.end(); ++hash1_it)
//     {
//         unordered_map<std::wstring, double>::const_iterator hash2_it = tf_idf_weights_hash2.find(hash1_it->first);
//         //we should not check hash2_it because we fill this container by fillTextVectorDiff and value must present

//             dot += hash1_it->second * hash2_it->second;
//             denom1 += hash1_it->second * hash1_it->second;
//             denom2 += hash2_it->second * hash2_it->second;

//     }

//     //cos(O) = a*b/(||a||*||b||)
//     return dot / (sqrt(denom1) * sqrt(denom2)) ;
// }

// void term_vector::findMissingWordAndFill(const unordered_map<std::wstring, double>& sourceTextVector, unordered_map<std::wstring, double>& destinationTextVector)
// {
//     for(unordered_map<std::wstring, double>::const_iterator it1 = sourceTextVector.begin(); it1 != sourceTextVector.end(); ++it1)
//     {
//         unordered_map<std::wstring, double>::iterator it2 = destinationTextVector.find(it1->first);
//         if(it2 == destinationTextVector.end())
//         {
//             destinationTextVector[it1->first] = m_missedValueWeight;
//         }
//     }
// }

// double term_vector::getSimilarityWith(const term_vector& vecObject)
// {
//     unordered_map<std::wstring, double> contentCopy1(m_tf);
//     unordered_map<std::wstring, double> contentCopy2(vecObject.m_tf);

//     fillTextVectorDiff(contentCopy1,contentCopy2);
//     //printTextVector(contentCopy1);

//     return cosineSimilarity(contentCopy1,contentCopy2);

// }

double term_vector::getSimilarity(const std::wstring& textString1, const std::wstring& textString2)
{
    (void)textString1;
    (void)textString2;
    return 0.0;
}

double term_vector::getSimilarity(const std::string& textString1, const std::string& textString2)
{
    (void)textString1;
    (void)textString2;
    return 0.0;
}

std::unordered_map<std::wstring, double>& term_vector::makeTermVector(const std::wstring& textString, std::unordered_map<std::wstring, double>& termVector)
{
    (void)textString;
    (void)termVector;
    return termVector;
}

std::unordered_map<std::string, double>& term_vector::makeTermVector(const std::string& textString, std::unordered_map<std::string, double>& termVector)
{
    (void)textString;
    (void)termVector;
    return termVector;
}

std::wstring term_vector::vectorToString(const std::unordered_map<std::wstring, double>& termVector)
{
    (void)termVector;
}

std::string term_vector::vectorToString(const std::unordered_map<std::string, double>& termVector)
{
    (void)termVector;
}

}}
