#pragma once

#ifndef MODEL_KEY_H_
#define MODEL_KEY_H_

class CModelKey
{
public:
    CModelKey() = default;

protected:
    std::string decode64(const std::string&);
};

#endif  // MODEL_KEY_H_