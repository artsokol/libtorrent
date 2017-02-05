#ifndef TERM_VECTOR_H
#define TERM_VECTOR_H

#include <iostream>
#include <string>
#include <fstream>

#include <boost/unordered_map.hpp>

#include <libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp>
#include "libtorrent/sha1_hash.hpp"

namespace libtorrent {
namespace nsw
{
/*
    Parse and store text as tf-idf vector (theoretical vector. storage is hash map)
    I have used common word tf-idf but idf always is 1 because this class for comparation for similarity two texts/documents.
*/
class term_vector
{
private:
    const double m_missedValueWeight; //using for filing frequency
    boost::unordered::unordered_map<std::wstring, double> m_tf;
    //double m_idf; //unnecceesary variable as we have just one document
    nsw_logger_observer_interface* m_observer;
public:

    term_vector():
        m_missedValueWeight(0.00000001),
        m_tf(),
        m_observer(NULL)
        //m_idf(1.0)
    {}

    term_vector(const char* textFile
                , nsw_logger_observer_interface* observer):
                        m_missedValueWeight(0.00000001),
                        m_tf(),
                        m_observer(observer)
    {
        int ret = parseFile(textFile);
#ifndef TORRENT_DISABLE_LOGGING
        // if(ret != 0)
        //     if (m_observer) m_observer->nsw_log(nsw_logger::node,
        //         "Term vector Error: object created but not ititialized correctly. File does not exist. %s",textFile);
#endif
    }

    term_vector(nsw_logger_observer_interface* observer,
                const std::string& string_to_parse):
                        m_missedValueWeight(0.00000001),
                        m_tf(),
                        m_observer(observer)
    {

        unsigned int wordCount = string2vec(string_to_parse);

#ifndef TORRENT_DISABLE_LOGGING
        // if(wordCount == 0)
        //     if (m_observer) m_observer->nsw_log(nsw_logger::node,
        //         "Term vector Error: object created but not ititialized correctly as string is empty%s",
        //                                                                 string_to_parse.c_str());
#endif
        for (boost::unordered::unordered_map<std::wstring, double>::iterator it = m_tf.begin(); wordCount != 0, it != m_tf.end(); ++it)
        {
            it->second /= wordCount;
        // cout << " key: " << std::get<0>(hashPair) << " |value: " << std::get<1>(hashPair) << endl;
        }

    }

    //term_vector(std::wstring& text);
    term_vector(const term_vector& copy):
        m_missedValueWeight(0.00000001)
    {
        if(this != &copy)
        {
            m_tf = copy.m_tf;
            m_observer = copy.m_observer;
        }
    }

    term_vector& operator=(const term_vector& copy)
    {
        if(this != &copy)
        {
            m_tf = copy.m_tf;
            return *this;
        }
    }

    ~term_vector(){};

    int parseFile(const char* file);
    unsigned int string2vec(const std::string& line);

    double getSimilarityWith(const std::wstring& textString);
    double getSimilarityWith(const term_vector& vecObject);

    //nuber of uniq words in our vector
    unsigned long cardinality() const;

    nsw_logger_observer_interface* observer() const { return m_observer; }

private:
    double cosineSimilarity(const boost::unordered::unordered_map<std::wstring, double>&,const boost::unordered::unordered_map<std::wstring, double>& );
    //
    void fillTextVectorDiff(boost::unordered::unordered_map<std::wstring, double>&,boost::unordered::unordered_map<std::wstring, double>& );
    //copy absent words from source vector to destination with missing value frequency
    void findMissingWordAndFill(const boost::unordered::unordered_map<std::wstring, double>& sourceTextVector, boost::unordered::unordered_map<std::wstring, double>& destinationTextVector);

    //for debug purpoces
    void printTextVector(const boost::unordered::unordered_map<std::wstring, double>& textVector)
    {
#ifndef TORRENT_DISABLE_LOGGING
        // if (!m_observer)
        //     return;
        // for ( unsigned i = 0; i < textVector.bucket_count(); ++i) {
        //     m_observer->nsw_log(nsw_logger::node, "bucket #%i  contains: ",i);
        //     for ( boost::unordered::unordered_map<std::wstring, double>::const_local_iterator local_it = textVector.begin(i); local_it!= textVector.end(i); ++local_it )
        //     {
        //         const std::string nstr( local_it->first.begin(),  local_it->first.end());
        //         m_observer->nsw_log(nsw_logger::node, " %s:  %lf",nstr.c_str(),local_it->second);
        //     }

        // }
#endif
    }


};

inline unsigned long term_vector::cardinality() const
{
    return m_tf.size();
}

inline void term_vector::fillTextVectorDiff(boost::unordered::unordered_map<std::wstring, double>& vec1,boost::unordered::unordered_map<std::wstring, double>& vec2)
{
    findMissingWordAndFill(vec1,vec2);

    findMissingWordAndFill(vec2,vec1);
}

} }

#endif // TERM_VECTOR_H
