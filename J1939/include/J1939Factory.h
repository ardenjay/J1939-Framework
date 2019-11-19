/*
 * J1939Factory.h
 *
 *  Created on: Sep 23, 2017
 *      Author: famez
 */


/*
MIT License

Copyright (c) 2018 Fernando Ámez García

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef J1939FACTORY_H_
#define J1939FACTORY_H_

#include <memory>
#include <map>
#include <set>
#include <Types.h>

#include <Singleton.h>
#include <J1939DataBase.h>

namespace J1939 {

class J1939Frame;

class J1939Factory : public ISingleton<J1939Factory> {

	SINGLETON_ACCESS;

	virtual ~J1939Factory();

private:
	J1939Factory();
	std::map<u32, J1939Frame*> mFrames;

	 /*
	 * Registers the predefined frames that we can find in J1939Protocol
	 */
	void registerPredefinedFrames();


public:

	/*
	 * Returns the corresponding frame (if registered) from the given id and decodes the information from data and length
	 */
    std::unique_ptr<J1939Frame> getJ1939Frame(u32 id, const u8* data, size_t length);
    /*
     * Returns the corresponding frame (if registered) from the given PGN
     */
    std::unique_ptr<J1939Frame> getJ1939Frame(u32 pgn);


    /*
	 * Returns the corresponding frame (if registered) from the given name
	 */
    std::unique_ptr<J1939Frame> getJ1939Frame(const std::string& name);


    /*
     * Registers the given frame in the factory letting the factory to create a copy of it and, if neccesary, decoding it.
     */
    bool registerFrame(const J1939Frame&);

    void unRegisterFrame(u32 pgn);

    bool registerDatabaseFrames(const std::string& ddbbFile);
    bool registerDatabaseFrames(J1939DataBase& db, const std::string path);

    void unregisterAllFrames();

	std::set<u32> getAllRegisteredPGNs() const;

};

} /* namespace J1939 */

#endif /* J1939FACTORY_H_ */
