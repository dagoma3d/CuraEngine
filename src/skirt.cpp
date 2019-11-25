/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include "skirt.h"
#include "support.h"

namespace cura {

void generateSkirt(SliceDataStorage& storage, int distance, int extrusionWidth, int count, int minLength, int initialLayerHeight, int additionalLines, bool external)
{
    Polygons baseWipeTower(storage.wipeTower);
    // Don't skirt holes
    baseWipeTower.clear();
    for(int i = 0; i < storage.wipeTower.size(); i++) {
      if(storage.wipeTower[i].area() > 0) baseWipeTower.add(storage.wipeTower[i]);
    }

    for(int skirtNr=0; skirtNr<count;skirtNr++)
    {
        int wipeTowerOffsetDistance = extrusionWidth * (skirtNr + 1);
        int offsetDistance = distance + extrusionWidth * skirtNr + extrusionWidth / 2;

        SupportPolyGenerator supportGenerator(storage.support, initialLayerHeight);
        Polygons skirtPolygons(baseWipeTower.offset(wipeTowerOffsetDistance));

        for(unsigned int volumeIdx = 0; volumeIdx < storage.volumes.size(); volumeIdx++)
        {
            if (storage.volumes[volumeIdx].layers.size() < 1) continue;
            SliceLayer* layer = &storage.volumes[volumeIdx].layers[0];
            for(unsigned int i=0; i<layer->parts.size(); i++)
            {
                if (external)
                {
                    Polygons p;
                    p.add(layer->parts[i].outline[0]);
                    skirtPolygons = skirtPolygons.unionPolygons(p.offset(offsetDistance));
                }
                else
                    skirtPolygons = skirtPolygons.unionPolygons(layer->parts[i].outline.offset(offsetDistance));

                supportGenerator.polygons = supportGenerator.polygons.difference(layer->parts[i].outline);
            }
        }

        //Contract and expand the suppory polygons so small sections are removed and the final polygon is smoothed a bit.
        supportGenerator.polygons = supportGenerator.polygons.offset(-extrusionWidth * 3);
        supportGenerator.polygons = supportGenerator.polygons.offset(extrusionWidth * 3);
        skirtPolygons = skirtPolygons.unionPolygons(supportGenerator.polygons.offset(offsetDistance));

        //Remove small inner skirt holes. Holes have a negative area, remove anything smaller then 100x extrusion "area"
        for(unsigned int n=0; n<skirtPolygons.size(); n++)
        {
            double area = skirtPolygons[n].area();
            if (area < 0 && area > -extrusionWidth * extrusionWidth * 100)
                skirtPolygons.remove(n--);
        }

        storage.skirt.add(skirtPolygons);

        int length = storage.skirt.polygonLength();
        if (skirtNr + 1 >= count && length > 0 && length < minLength)
            count++;
    }

    // Add skirts for the wipe tower
    if(additionalLines > 0)
    {
      for(int i = 1; i <= additionalLines; i++) {
        Polygons skirtPolygons(baseWipeTower.offset(extrusionWidth * (count + i)));
        storage.skirt.add(skirtPolygons);
      }
    }
}

}//namespace cura
