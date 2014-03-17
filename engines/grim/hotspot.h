/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef HOTSPOT_H_
#define HOTSPOT_H_

#include "common/rect.h"
#include "common/array.h"
#include "primitives.h"
#include "common/hashmap.h"
#include "math/vector3d.h"
#include "common/events.h"
#include "common/hash-str.h"

namespace Grim {

class GrimEngine;
class Cursor;
class SaveGame;

struct Polygon {
    Common::Array<Common::Point> _pnts;
    
    void draw(const Color &col);
    bool contains(const Common::Point& pos);
    void move(const Common::Point& center);
};

struct HotObject {
    Common::String _id, _desc;
    Math::Vector3d _pos;
    Common::Rect _rect;    
    bool _active;
};

struct Hotspot {
	Common::String _id, _desc;
    Polygon _region;
    Math::Vector3d _pos;
    int _setup;
    int _type;
    int _objId;
};

struct InventoryItem {
    Common::String _id;
    Bitmap* _bmp;
};

class HotspotMan {
public:
    HotspotMan();
    virtual ~HotspotMan();

    enum ControlMode { Normal=0, Dialog=1, Special=2, Linear=3, Inventory=4, NoWalk=5 };
    
    int addHotspot(const Common::String& name, const Math::Vector3d& pos, const Common::String& scene);    
    void disableAll();
    HotObject& getObject(int idx) { return _hotobject[idx]; }
    
    void okKey();
    void cancel();
    void event(const Common::Point& cursor, const Common::Event& ev, int debug);
    void getName(Cursor* cursor);
    void hover(Cursor* cursor);
    void updatePerspective();
    void drawActive(int debug);    
    void reload(bool always);
    bool restoreState(SaveGame *savedState);
    void saveState(SaveGame* savedState);
    void switchMode(int ctrlMode) { _ctrlMode = ctrlMode; }
    void setupDialog(int x0, int y0, int w, int h, int rows, int cols) { _x0=x0; _y0=y0; _w=w; _h=h; _cols=cols; _rows=rows; }
    void notifyWalkOut();
    void debug(int num);
    void update();
    void resetInventory();
    void addInventory(const Common::String& id, const Common::String& pic);
    void updateHotspot(const Common::String& id, const Math::Vector3d& pos, int vis);
    int getCtrlMode() { return _ctrlMode; }
    void setAxis(const Math::Vector3d& a, float offset) { _axis = a; _offset=offset; }
protected:
    void append_hotspot(const Common::String& id, const Common::String& name, int type, const Math::Vector3d& v);
    int inBox(const Common::Point& p);
    void freeClick(const Common::Point& cursor, int button, bool doubleClick, bool climbing);

    // dialog support
    int _ctrlMode, _rows, _cols;
    int _x0,_y0,_w,_h;

    // inventory
    Common::HashMap<Common::String, Bitmap*> _icons;
    Common::Array<InventoryItem> _inventory;

    // linear mode
    Math::Vector3d _axis;
    float _offset;

    unsigned int _lastClick;
    bool _initialized;
    Common::HashMap<String,Hotspot> _hotspots;
    Common::Array<Hotspot> _hotspot;
    Common::String _curScene;
    Common::Array<HotObject> _hotobject;
    
    // hotspot editing
    int _selectMode, _lastSetup;
    Polygon _selectPoly;
    Common::String _lastName;        
};

} /* namespace  */
#endif /* HOTSPOT_H_ */
