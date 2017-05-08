#ifndef TERM_VECTOR_H
#define TERM_VECTOR_H

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>

#include <boost/unordered_map.hpp>

#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"
#include "libtorrent/entry.hpp"
#include "libtorrent/sha1_hash.hpp"

namespace libtorrent {
namespace nsw
{
using vector_t = std::unordered_map<std::string, double>;
using wvector_t = std::unordered_map<std::wstring, double>;

#define ZERO_FREQUENCY_CONST (0.00000001)
class term_vector
{
public:

//    nsw_logger_observer_interface* observer() const { return m_observer; }

    static double getSimilarity(const std::wstring& textString1, const std::wstring& textString2);
    static double getSimilarity(const std::string& textString1, const std::string& textString2);

    static double getVecSimilarity(const wvector_t& textVec1, const wvector_t& textVec2);

    static double getVecSimilarity(const vector_t& textVec1, const vector_t& textVec2);

    static wvector_t& makeTermVector(const std::wstring& textString, wvector_t& termVector);
    static vector_t& makeTermVector(const std::string& textString, vector_t& termVector);

    static std::wstring vectorToString(const wvector_t& termVector);
    static std::string vectorToString(const vector_t& termVector);

    static entry& vectorToEntry(const vector_t& termVector, entry& a);
private:

    static void alignVectors(wvector_t& tv1, wvector_t& tv2);
    static void alignVectors(vector_t& tv1, vector_t& tv2);

    static double hardCosineMeasure(const wvector_t& textVec1, const wvector_t& textVec2);
    static double hardCosineMeasure(const vector_t& textVec1, const vector_t& textVec2);

    static double softCosineMeasure(const wvector_t& textVec1, const wvector_t& textVec2);
    static double softCosineMeasure(const vector_t& textVec1, const vector_t& textVec2);
    static size_t levenshteinDist(const std::wstring& textString, const std::wstring& termVector);
    static size_t levenshteinDist(const std::string& textString, const std::string& termVector);

    //for debug purpoces
//     void printTextVector(const boost::unordered::unordered_map<std::wstring, double>& textVector)
//     {
// #ifndef TORRENT_DISABLE_LOGGING
//         // if (!m_observer)
//         //     return;
//         // for ( unsigned i = 0; i < textVector.bucket_count(); ++i) {
//         //     m_observer->nsw_log(nsw_logger::node, "bucket #%i  contains: ",i);
//         //     for ( boost::unordered::unordered_map<std::wstring, double>::const_local_iterator local_it = textVector.begin(i); local_it!= textVector.end(i); ++local_it )
//         //     {
//         //         const std::string nstr( local_it->first.begin(),  local_it->first.end());
//         //         m_observer->nsw_log(nsw_logger::node, " %s:  %lf",nstr.c_str(),local_it->second);
//         //     }

//         // }
// #endif
//     }


};

} }

#endif // TERM_VECTOR_H
