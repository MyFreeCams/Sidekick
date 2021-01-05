
//system
#include <string>

// project
#include "ModelKey.h"

// boost libraries
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <algorithm> 

#include <boost/algorithm/string.hpp>




std::string CModelKey::decode64(const std::string& s) {
    using namespace boost::archive::iterators;
    typedef transform_width<binary_from_base64<remove_whitespace
        <std::string::const_iterator> >, 8, 6> ItBinaryT;

    std::string input = s;
    try
    {
        // If the input isn't a multiple of 4, pad with =
        size_t num_pad_chars((4 - input.size() % 4) % 4);
        input.append(num_pad_chars, '=');

        size_t pad_chars(std::count(input.begin(), input.end(), '='));
        std::replace(input.begin(), input.end(), '=', 'A');
        std::string output(ItBinaryT(input.begin()), ItBinaryT(input.end()));
        output.erase(output.end() - pad_chars, output.end());
        return output;
    }
    catch (std::exception const&ex)
    {
        return std::string("");
    }

}


