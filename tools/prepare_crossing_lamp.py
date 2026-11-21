"""Author a small emissive red signal lens; no collision geometry."""
import math
import struct
from pathlib import Path


def prepare(folder: Path):
    count = 24
    rings = 14
    radius = 0.155
    points = [(0,0,0)]
    for ring in range(1,rings+1):
        r=radius*ring/rings
        points.extend((r*math.cos(i*2*math.pi/count),r*math.sin(i*2*math.pi/count),0) for i in range(count))
    faces=[]
    for ring in range(1,rings+1):
        midpoint=radius*(ring-0.5)/rings
        alpha=math.exp(-midpoint*midpoint/(2*0.055*0.055))
        for i in range(count):
            first=1+(ring-1)*count+i
            second=1+(ring-1)*count+(i+1)%count
            triangles=[(0,second,first)]
            if ring>1:
                inner=first-count; inner_next=second-count
                triangles=[(inner,second,first),(inner,inner_next,second)]
            for triangle in triangles:
                faces.extend([(triangle,alpha),(tuple(reversed(triangle)),alpha)])
    lod=b"P3DM"+struct.pack("<6I",28,256,len(points),2,len(faces),0)
    lod+=b"".join(struct.pack("<3fI",*point,0) for point in points)
    lod+=struct.pack("<6f",0,0,-1,0,0,1)
    for index,(face,alpha) in enumerate(faces):
        lod+=struct.pack("<I",3)
        for vertex in face: lod+=struct.pack("<II2f",vertex,index%2,0,0)
        lod+=bytes(16)+struct.pack("<I",0)
        texture=f"#(argb,8,8,3)color(1,0,0,{alpha:.6f},CO)"
        lod+=texture.encode('ascii')+b"\0RailPilot\\Crossings\\models\\rp_crossing_lamp.rvmat\0"
    lod+=b"TAGG\1#EndOfFile#\0"+struct.pack("<If",0,1)
    (folder/"rp_crossing_lamp.p3d").write_bytes(b"MLOD"+struct.pack("<II",257,1)+lod)
