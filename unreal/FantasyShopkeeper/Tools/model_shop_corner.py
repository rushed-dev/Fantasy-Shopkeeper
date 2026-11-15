"""Build the original shop kit with Blender Python (bpy), in centimetres.

Run with Blender --background --python, or Python with bpy on PYTHONPATH.
Produces editable .blend, per-asset FBX with authored UCX collision, and a preview.
"""

import json
import math
from pathlib import Path

import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[3]
OUTPUT = ROOT / "art/shop-corner"
OUTPUT.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = 0.01

PALETTE = {
    "HoneyWood": ((0.34, 0.15, 0.055, 1), 0.48, 0),
    "DarkWood": ((0.14, 0.055, 0.022, 1), 0.57, 0),
    "SagePaint": ((0.24, 0.32, 0.19, 1), 0.55, 0),
    "Brass": ((0.53, 0.32, 0.095, 1), 0.29, 0.72),
    "AmberGlass": ((0.55, 0.22, 0.025, 1), 0.16, 0),
    "TonicLiquid": ((0.9, 0.34, 0.025, 1), 0.23, 0),
    "Cork": ((0.33, 0.18, 0.075, 1), 0.85, 0),
    "CreamLabel": ((0.88, 0.77, 0.51, 1), 0.8, 0),
    "LabelInk": ((0.09, 0.13, 0.055, 1), 0.8, 0),
    "CreamCeramic": ((0.75, 0.65, 0.43, 1), 0.3, 0),
    "MoonleafGreen": ((0.15, 0.29, 0.095, 1), 0.65, 0),
    "WaterGlass": ((0.2, 0.51, 0.62, 1), 0.15, 0),
    "LampGlow": ((1, 0.65, 0.2, 1), 0.3, 0),
    "WarmPlaster": ((0.63, 0.52, 0.36, 1), 0.92, 0),
    "FloorWood": ((0.22, 0.105, 0.045, 1), 0.76, 0),
    "WindowGlow": ((0.32, 0.57, 0.68, 1), 0.35, 0),
    "BookCloth": ((0.15, 0.23, 0.23, 1), 0.85, 0),
    "InkGlass": ((0.045, 0.19, 0.32, 1), 0.2, 0),
    "InkLiquid": ((0.025, 0.26, 0.58, 1), 0.26, 0),
}
materials = {}
for name, (color, roughness, metallic) in PALETTE.items():
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = color
    shader.inputs["Roughness"].default_value = roughness
    shader.inputs["Metallic"].default_value = metallic
    if name in ("AmberGlass", "InkGlass"):
        shader.inputs["Transmission Weight"].default_value = 0.65
        shader.inputs["IOR"].default_value = 1.45
    if name in ("HoneyWood", "DarkWood", "Cork"):
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        coord = nodes.new("ShaderNodeTexCoord")
        stretch = nodes.new("ShaderNodeVectorMath")
        stretch.operation = "MULTIPLY"
        stretch.inputs[1].default_value = (3, 90, 90)
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 2
        noise.inputs["Detail"].default_value = 2
        ramp = nodes.new("ShaderNodeValToRGB")
        ramp.color_ramp.elements[0].color = tuple(v * 0.7 for v in color[:3]) + (1,)
        ramp.color_ramp.elements[1].color = tuple(v * 1.12 for v in color[:3]) + (1,)
        links.new(coord.outputs["Generated"], stretch.inputs[0])
        links.new(stretch.outputs["Vector"], noise.inputs["Vector"])
        links.new(noise.outputs["Fac"], ramp.inputs[0])
        links.new(ramp.outputs["Color"], shader.inputs["Base Color"])
    materials[name] = mat


def finish(obj, name, material, bevel=0):
    obj.name = name
    obj.data.materials.append(materials[material])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod = obj.modifiers.new("Soft handmade edges", "BEVEL")
        mod.width = bevel
        mod.segments = 3
        bpy.ops.object.modifier_apply(modifier=mod.name)
        normal = obj.modifiers.new("Weighted corner normals", "WEIGHTED_NORMAL")
        bpy.ops.object.modifier_apply(modifier=normal.name)
    return obj


def box(name, center, size, material="HoneyWood", bevel=0.5):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.dimensions = size
    return finish(obj, name, material, bevel)


def lathe(name, profile, material, depth_ratio=1, power=2, segments=48):
    vertices = []
    for radius, z in profile:
        for i in range(segments):
            angle = math.tau * i / segments
            x, y = math.cos(angle), math.sin(angle)
            vertices.append((radius * math.copysign(abs(x) ** (2 / power), x),
                             radius * depth_ratio * math.copysign(abs(y) ** (2 / power), y), z))
    faces = []
    for row in range(len(profile) - 1):
        for i in range(segments):
            a = row * segments + i
            b = row * segments + (i + 1) % segments
            faces.append((a, b, b + segments, a + segments))
    faces.append(tuple(reversed(range(segments))))
    faces.append(tuple((len(profile) - 1) * segments + i for i in range(segments)))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(obj)
    obj.data.materials.append(materials[material])
    for face in mesh.polygons:
        face.use_smooth = len(face.vertices) == 4
    return obj


def line(name, points, radius, material):
    curve = bpy.data.curves.new(name, "CURVE")
    curve.dimensions = "3D"
    curve.bevel_depth = radius
    curve.bevel_resolution = 1
    spline = curve.splines.new("POLY")
    spline.points.add(len(points) - 1)
    for p, co in zip(spline.points, points):
        p.co = (*co, 1)
    obj = bpy.data.objects.new(name, curve)
    scene.collection.objects.link(obj)
    obj.data.materials.append(materials[material])
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    obj.select_set(False)
    return obj


def text(body, size, center):
    bpy.ops.object.text_add(location=center, rotation=(math.pi / 2, 0, 0))
    obj = bpy.context.object
    obj.data.body = body
    obj.data.align_x = "CENTER"
    obj.data.size = size
    obj.data.extrude = 0.006
    obj.data.resolution_u = 4
    obj.data.materials.append(materials["LabelInk"])
    bpy.ops.object.convert(target="MESH")
    return obj


assets = {}


def export_asset(name, parts, collisions):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    # Bake the front for Unreal's FBX handedness conversion into the geometry.
    obj.rotation_euler[2] = math.pi
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    # UVs support future texture authoring and let Unreal generate lightmap UVs.
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    collision_objects = []
    for i, (center, size) in enumerate(collisions):
        hull = box(f"UCX_{name}_{i:02d}", (-center[0], -center[1], center[2]), size, bevel=0)
        collision_objects.append(hull)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    for hull in collision_objects:
        hull.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=str(OUTPUT / f"{name}.fbx"), use_selection=True,
                             object_types={"MESH"}, axis_forward="-Y", axis_up="Z",
                             apply_unit_scale=True, bake_anim=False, use_mesh_modifiers=True,
                             mesh_smooth_type="FACE")
    for hull in collision_objects:
        hull.parent = obj
        hull.matrix_parent_inverse = obj.matrix_world.inverted()
        hull.hide_render = True
        hull.hide_set(True)
    triangles = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    assets[name] = dict(triangles=triangles, dimensions_cm=list(obj.dimensions),
                        collision_hulls=len(collisions), materials=[m.name for m in obj.data.materials])
    return obj


# The inserted cork follows the flattened opening. Its wider head starts above
# the 23.8 cm glass lip, with clearance around the inserted section.
cork_profile = [(1.7, 22.1), (1.85, 23.6), (1.9, 23.95),
                (2.6, 24.05), (2.7, 24.7), (2.4, 25)]
assert all(radius <= 1.9 for radius, z in cork_profile if z <= 23.8)
assert next(z for radius, z in cork_profile if radius > 2.2) > 23.8

# Squared, softly shouldered 14 x 10 x 25 cm bottle with a bottom-centre pivot.
parts = [lathe("BottleShell", [(5.5, 0), (6.7, 0.5), (7, 1.2), (7, 14.5),
          (6.8, 16), (5.4, 18.2), (2.7, 20), (2.4, 20.5), (2.4, 22.5),
          (3, 22.7), (3, 23.8), (2.2, 23.8), (2, 22), (2, 20.5),
          (5.1, 17.8), (6.3, 15.6), (6.5, 1.2)], "AmberGlass", 0.73, 3.6),
         lathe("GoldenTonic", [(5.8, 0.9), (6.25, 1.4), (6.25, 14), (6.1, 14.3)], "TonicLiquid", 0.73, 3.6),
         lathe("CorkStopper", cork_profile, "Cork", 0.73, 3.6),
         box("PaperLabel", (0, -5.16, 9.2), (9.7, 0.14, 10.6), "CreamLabel", 0.35)]
parts += [text("DAWN", 1.65, (0, -5.26, 12.1)), text("TONIC", 1.35, (0, -5.26, 10.3))]
sun = [(1.15 * math.cos(i * math.tau / 32), -5.28, 7.6 + 1.15 * math.sin(i * math.tau / 32)) for i in range(33)]
parts.append(line("SunSeal", sun, 0.06, "Brass"))
for i in range(12):
    a = math.tau * i / 12
    parts.append(line("SunRay", [(r * math.cos(a), -5.28, 7.6 + r * math.sin(a)) for r in (1.5, 1.9)], 0.055, "LabelInk"))
for side in (-1, 1):
    parts.append(line("BotanicalStem", [(side * (2.1 + 0.18 * i), -5.28, 5.2 + 0.35 * i) for i in range(8)], 0.04, "LabelInk"))
    for i in range(4):
        x, z = side * (2.3 + i * 0.3), 5.6 + i * 0.6
        parts.append(line("Leaf", [(x, -5.28, z), (x + side * 0.7, -5.28, z + 0.7), (x + side * 0.1, -5.28, z + 0.6)], 0.07, "LabelInk"))
bottle = export_asset("SM_DawnTonic", parts, [((0, 0, 9), (14, 10.6, 18)), ((0, 0, 21), (6, 6, 8))])

parts = [lathe("InkBottleShell", [(6, 0), (7.5, 0.7), (8, 2), (8, 13), (7.5, 15), (5, 17.5),
          (2.5, 20), (2.4, 22.5), (3, 22.7), (3, 23.8), (2.2, 23.8), (2, 22), (2, 20.5),
          (4.6, 17), (7.2, 13), (7.2, 1.4)], "InkGlass", 0.8, 4),
         lathe("LuminousInk", [(6.8, 1), (7, 1.5), (7, 12.5)], "InkLiquid", 0.8, 4),
         lathe("InkCork", cork_profile, "Cork", 0.8, 4),
         box("InkPaperLabel", (0, -6.43, 8.5), (10, 0.14, 11), "CreamLabel", 0.4),
         text("MOONLIT", 1.6, (0, -6.54, 11.8)), text("INK", 1.6, (0, -6.54, 9.5))]
outer_angle = math.acos(0.948076923 / 1.5)
inner_angle = math.acos((0.948076923 - 0.65) / 1.2)
points = [(1.5 * math.cos(a), -6.56, 6.5 + 1.5 * math.sin(a))
          for a in [outer_angle + (math.tau - 2 * outer_angle) * i / 32 for i in range(33)]]
points += [(0.65 + 1.2 * math.cos(a), -6.56, 6.5 + 1.2 * math.sin(a))
           for a in [math.tau - inner_angle - (math.tau - 2 * inner_angle) * i / 32 for i in range(33)]]
parts.append(line("CrescentSeal", points, 0.1, "LabelInk"))
ink_bottle = export_asset("SM_MoonlitInk", parts, [((0, 0, 8), (16, 12.8, 16)), ((0, 0, 20), (6, 6, 10))])

# 220 x 100 cm workbench. Top at 100 cm matches the existing brewing station.
parts, hulls = [], []
for i in range(5):
    parts.append(box("WorkbenchTopPlank", (0, -40 + i * 20, 97), (220, 19.7, 6), bevel=1.1))
hulls.append(((0, 0, 97), (220, 100, 6)))
for x in (-96, 96):
    for y in (-36, 36):
        parts.append(box("TaperedLeg", (x, y, 47), (11, 11, 94), bevel=1.2))
        parts.append(box("LegFoot", (x, y, 4), (13, 13, 8), "DarkWood", 0.8))
        hulls.append(((x, y, 47), (13, 13, 94)))
for y in (-38, 38):
    parts.append(box("Apron", (0, y, 85), (190, 5, 16), bevel=0.7))
    hulls.append(((0, y, 85), (190, 5, 16)))
    parts.append(box("SageApronInset", (0, y + math.copysign(2.7, y), 85), (170, 0.8, 8), "SagePaint", 0.3))
for i in range(4):
    parts.append(box("LowerShelfPlank", (0, -27 + i * 18, 22), (190, 17.7, 4), bevel=0.6))
hulls.append(((0, 0, 22), (190, 72, 4)))
for x in (-96, 96):
    parts.append(box("EndApron", (x, 0, 84), (5, 72, 16), bevel=0.7))
    hulls.append(((x, 0, 84), (5, 72, 16)))
    for y in (-42, 42):
        peg = lathe("BrassJoineryPeg", [(1.3, 0), (1.3, 0.4)], "Brass", segments=16)
        peg.rotation_euler[0] = math.pi / 2
        peg.location = (x, y, 86)
        parts.append(peg)
bench = export_asset("SM_ApothecaryBench", parts, hulls)

# Shelving has four usable surfaces, physical board collision and an open front.
parts, hulls = [], []
for x in (-56, 56):
    for y in (-19, 18):
        parts.append(box("ShelfUpright", (x, y, 92), (8, 8, 184), bevel=0.9))
        hulls.append(((x, y, 92), (8, 8, 184)))
for z in (20, 60, 100, 140):
    parts.append(box("ShelfBoard", (0, 0, z - 2), (112, 44, 4), bevel=0.65))
    parts.append(box("ShelfFrontTrim", (0, -22.5, z - 4), (104, 2, 6), "DarkWood", 0.4))
    hulls.append(((0, 0, z - 2), (112, 44, 4)))
for i in range(8):
    parts.append(box("BackingPlank", (-49 + i * 14, 21, 96), (13.7, 2, 164), "SagePaint", 0.3))
hulls.append(((0, 21, 96), (112, 2, 164)))
parts.append(box("CrownRail", (0, 0, 184), (124, 48, 8), bevel=1))
hulls.append(((0, 0, 184), (124, 48, 8)))
parts.append(box("Crest", (0, 19, 191), (78, 5, 10), bevel=3))
for x in (-56, 56):
    for z in (24, 64, 104, 144, 184):
        peg = lathe("ShelfBrassPeg", [(0.9, 0), (0.9, 0.3)], "Brass", segments=12)
        peg.rotation_euler[0] = math.pi / 2
        peg.location = (x, -22.1, z)
        parts.append(peg)
shelf = export_asset("SM_ApothecaryShelving", parts, hulls)

# Brass and ceramic brewer with a visible working port and sturdy short feet.
parts = [lathe("CeramicVessel", [(13, 6), (18, 9), (20, 16), (20, 23), (18, 26)], "CreamCeramic"),
         lathe("BrassLid", [(20.5, 23), (21, 25), (19, 27), (16, 33), (9, 37), (2, 38)], "Brass"),
         lathe("LidKnob", [(2, 38), (3.5, 39), (3.5, 41), (2, 42)], "DarkWood")]
for x, y in ((-13, -10), (13, -10), (0, 13)):
    parts.append(box("BrewerFoot", (x, y, 5), (5, 5, 10), "Brass", 1))
for side in (-1, 1):
    parts.append(line("BrewerHandle", [(side * 18, 0, 25), (side * 26, 0, 26),
                                       (side * 28, 0, 20), (side * 20, 0, 18)], 1.4, "Brass"))
port = lathe("WorkingPort", [(4.4, 0), (4.4, 0.7)], "WaterGlass")
port.rotation_euler[0] = math.pi / 2
port.location = (0, -19, 27)
parts.append(port)
parts.append(line("PortRim", [(5.1 * math.cos(i * math.tau / 32), -19.8, 27 + 5.1 * math.sin(i * math.tau / 32)) for i in range(33)], 0.65, "Brass"))
brewer = export_asset("SM_BrewingVessel", parts, [((0, 0, 22), (40, 40, 40))])

parts = [lathe("MoonleafJar", [(7, 0), (9, 2), (10, 8), (9, 18), (7, 21), (7.8, 22), (7.8, 23), (6.5, 23), (6, 20), (8.3, 17), (8.7, 8), (6, 2)], "SagePaint"),
         lathe("JarLid", [(7, 23.5), (8.2, 24), (8, 25), (2, 26), (2, 27.5)], "CreamCeramic")]
for side in (-1, 1):
    points = [(side * 2, -9.8, 6), (side * 5, -9.8, 10), (side * 3, -9.8, 15), (side, -9.8, 10), (side * 2, -9.8, 6)]
    parts.append(line("LeafJarEmblem", points, 0.3, "CreamLabel"))
jar = export_asset("SM_MoonleafJar", parts, [((0, 0, 14), (20, 20, 28))])

parts = [lathe("WaterJug", [(7, 0), (9, 2), (10, 7), (9, 18), (6, 23), (4, 24), (4, 28), (5.2, 29), (3.3, 29), (3.2, 24), (5.3, 22), (8.2, 17), (8.8, 3)], "WaterGlass"),
         lathe("JugStopper", [(3, 27), (3.2, 29), (3.7, 30), (3.5, 32)], "Cork"),
         line("JugHandle", [(8, 0, 22), (15, 0, 22), (17, 0, 17), (16, 0, 10), (9, 0, 8)], 1.3, "WaterGlass")]
jug = export_asset("SM_SpringWaterJug", parts, [((0, 0, 16), (20, 20, 32))])

parts = [lathe("MortarBowl", [(6, 0), (8, 1), (10, 5), (11, 12), (10, 13), (9, 11), (8, 5), (5, 3)], "CreamCeramic")]
pestle = lathe("WoodenPestle", [(2, 0), (2.5, 2), (1.3, 6), (1.1, 23), (1.6, 25)], "DarkWood")
pestle.rotation_euler[1] = math.radians(25)
pestle.location = (0, 0, 4)
parts.append(pestle)
mortar = export_asset("SM_MortarAndPestle", parts, [((0, 0, 7), (22, 22, 14))])

parts = [lathe("LampShade", [(23, 0), (24, 1), (22, 3), (17, 10), (9, 17), (4, 20), (4, 23)], "Brass"),
         lathe("LampDiffuser", [(19, 0.3), (19, 0.6)], "LampGlow"),
         line("LampSuspension", [(0, 0, 23), (0, 0, 58)], 0.6, "DarkWood")]
lamp = export_asset("SM_BrassPendant", parts, [((0, 0, 11.5), (48, 48, 23))])

parts = [box("TillChest", (0, 0, 8), (55, 32, 16), "HoneyWood", 1.2),
         box("TillTop", (0, 0, 17), (56, 33, 2), "DarkWood", 0.6),
         box("DrawerFront", (0, -16.2, 8), (47, 1, 11), "SagePaint", 0.5),
         box("DrawerHandle", (0, -17.4, 8), (17, 1.8, 1.8), "Brass", 0.5),
         box("TillBackboard", (0, 10, 27), (45, 5, 18), "HoneyWood", 1.2),
         box("TillSign", (0, 7.2, 27), (39, 0.5, 12), "CreamLabel", 0.7),
         text("TILL", 4, (0, 6.8, 25.6))]
for x in (-17, 0, 17):
    cup = lathe("CoinWell", [(5, 18), (5, 19.2), (4.3, 19.2), (4.3, 18.3)], "Brass", segments=32)
    cup.location = (x, -5, 0)
    parts.append(cup)
till = export_asset("SM_ApothecaryTill", parts, [((0, 0, 9), (56, 33, 18)), ((0, 10, 27), (45, 5, 18))])

# A labelled, physical supplier station. Its contents are decorative, while the
# native crate transfers the entire advertised parcel through one transaction.
parts = [box("CrateBase", (0, 0, 4), (70, 50, 8), "DarkWood", 1)]
for x in (-32, 32):
    parts.append(box("CrateEnd", (x, 0, 28), (6, 50, 48), "HoneyWood", 0.7))
for y in (-23, 23):
    for z in (15, 32, 49):
        parts.append(box("CrateSlat", (0, y, z), (64, 4, 14), "HoneyWood", 0.6))
    parts.append(box("CrateCornerBand", (-27, y - math.copysign(2.2, y), 28), (5, 0.8, 48), "Brass", 0.2))
    parts.append(box("CrateCornerBand", (27, y - math.copysign(2.2, y), 28), (5, 0.8, 48), "Brass", 0.2))
parts += [box("SupplierLabel", (0, -25.5, 31), (43, 0.7, 26), "CreamLabel", 0.7),
          text("SUPPLIES", 4, (0, -26, 37)),
          text("6 MOONLEAF", 2.3, (0, -26, 31)),
          text("3 WATER", 2.3, (0, -26, 26)),
          text("12 BITS", 2.4, (0, -26, 20))]
for x in (-15, 15):
    jar_body = lathe("SealedSupplyJar", [(8, 8), (10, 12), (10, 51), (7, 56), (7, 63)], "CreamCeramic", segments=32)
    jar_body.location.x = x
    parts.append(jar_body)
    cap = lathe("SupplyJarCap", [(8, 62), (8, 66), (6, 67)], "SagePaint", segments=24)
    cap.location.x = x
    parts.append(cap)
crate = export_asset("SM_SupplyCrate", parts, [((0, 0, 26), (70, 50, 52)), ((0, 0, 58), (50, 24, 20))])

# Repeating floor boards have a single flat collision surface. The bottom of
# this tile sits below its origin so the existing shop floor height is retained.
parts = []
for row in range(5):
    for column in range(2):
        parts.append(box("OakFloorBoard", (-50 + column * 100, -40 + row * 20, -1.5), (99.6, 19.7, 3), "FloorWood", 0.18))
floor_tile = export_asset("SM_OakFloorTile", parts, [((0, 0, -1.5), (200, 100, 3))])

# Interior-facing panels fit one 100 cm greybox wall column. The framing sits
# proud of plaster and repeats without a heavy upright at every board seam.
parts = [box("LimePlaster", (0, 0, 150), (100, 6, 300), "WarmPlaster", 0.3),
         box("WallSkirting", (0, -4, 8), (100, 8, 16), "DarkWood", 0.6),
         box("ChairRail", (0, -4, 100), (100, 7, 7), "HoneyWood", 0.6),
         box("CeilingRail", (0, -4, 289), (100, 10, 22), "DarkWood", 0.8)]
wall_panel = export_asset("SM_PlasterWallPanel", parts, [((0, 0, 150), (100, 8, 300))])
parts = [box("TimberUpright", (0, 0, 150), (13, 16, 300), "DarkWood", 1.1)]
wall_post = export_asset("SM_TimberPost", parts, [((0, 0, 150), (13, 16, 300))])

# Arched, leaded glazing is a cool luminous backdrop for the warm workbench.
# This kit supplies a window surround. Exterior scenery can be authored later.
arch_points = [(70 * math.cos(i * math.pi / 32), 0, 120 + 70 * math.sin(i * math.pi / 32)) for i in range(33)]
outline = [(-70, 0, 0), (70, 0, 0)] + arch_points
mesh = bpy.data.meshes.new("ArchedPane")
mesh.from_pydata(outline, [], [tuple(range(len(outline)))])
mesh.update()
pane = bpy.data.objects.new("CoolWindowPane", mesh)
scene.collection.objects.link(pane)
pane.data.materials.append(materials["WindowGlow"])
parts = [pane, line("CarvedArch", [(-74, -3, 0), (-74, -3, 120)]
                  + [(74 * math.cos(math.pi - i * math.pi / 32), -3, 120 + 74 * math.sin(math.pi - i * math.pi / 32)) for i in range(33)]
                  + [(74, -3, 0)], 5, "HoneyWood"),
         box("WindowSill", (0, -9, 0), (164, 33, 9), "HoneyWood", 1.3),
         box("WindowMullion", (0, -3, 92), (4, 4, 184), "DarkWood", 0.4),
         box("WindowCrossbar", (0, -3, 105), (140, 4, 4), "DarkWood", 0.4)]
for x in (-35, 35):
    parts.append(box("LeadedVertical", (x, -2, 75), (1, 1, 150), "Brass", 0.1))
for z in (35, 70, 140):
    width = 140 if z < 120 else 2 * math.sqrt(70 ** 2 - (z - 120) ** 2)
    parts.append(box("LeadedHorizontal", (0, -2, z), (width, 1, 1), "Brass", 0.1))
window = export_asset("SM_ArchedWindow", parts, [((0, 0, 90), (150, 10, 190)), ((0, -9, 0), (164, 33, 9))])

parts = [box("RecipePages", (0, 0, 2), (40, 30, 3), "CreamLabel", 0.6),
         box("BookCover", (0, 0, 0.6), (43, 33, 1.2), "BookCloth", 0.7),
         line("BookSpine", [(0, -15, 3.6), (0, 15, 3.6)], 0.3, "DarkWood")]
# Text is laid flat with its top towards the back of the workbench.
for body, size, position in [("DAWN TONIC", 2.8, (0, 8, 3.65)),
                             ("2 MOONLEAF", 2.4, (0, 1, 3.65)),
                             ("1 SPRING WATER", 2, (0, -5, 3.65))]:
    lettering = text(body, size, position)
    lettering.rotation_euler = (0, 0, 0)
    parts.append(lettering)
book = export_asset("SM_DawnRecipeBook", parts, [((0, 0, 2), (43, 33, 4))])


def leaf(name, center, length, width, angle, material="MoonleafGreen"):
    """A thick folded leaf keeps its silhouette from both sides."""
    x, y, z = center
    direction = Vector((math.cos(angle), math.sin(angle), 0))
    side = Vector((-math.sin(angle), math.cos(angle), 0))
    base = Vector((x, y, z))
    middle = base + direction * length * 0.48 + Vector((0, 0, length * 0.17))
    tip = base + direction * length + Vector((0, 0, length * 0.08))
    vertices = [base, middle + side * width, tip, middle - side * width,
                middle + Vector((0, 0, 0.6)), middle - Vector((0, 0, 0.4))]
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], [(0, 1, 4), (1, 2, 4), (2, 3, 4), (3, 0, 4),
                                   (1, 0, 5), (2, 1, 5), (3, 2, 5), (0, 3, 5)])
    data.update()
    obj = bpy.data.objects.new(name, data)
    scene.collection.objects.link(obj)
    obj.data.materials.append(materials[material])
    return obj


parts = [line("TwineHanger", [(0, 0, 62), (0, 0, 70)], 0.5, "Cork"),
         line("HerbTie", [(-4, -3, 60), (4, -3, 60), (4, 3, 60), (-4, 3, 60), (-4, -3, 60)], 0.7, "Cork")]
for stem in range(7):
    angle = stem * math.tau / 7
    x, y = math.cos(angle) * 4, math.sin(angle) * 4
    parts.append(line("DryingStem", [(0, 0, 63), (x, y, 35), (x * 1.7, y * 1.7, 5)], 0.45, "MoonleafGreen"))
    for tier in range(5):
        parts.append(leaf("DryingLeaf", (x, y, 12 + tier * 8), 10, 2, angle + tier * 0.7))
herbs = export_asset("SM_DryingMoonleaf", parts, [((0, 0, 36), (25, 25, 70))])

parts = [lathe("FernPot", [(12, 0), (14, 2), (17, 22), (18, 24), (18, 27), (15, 27), (15, 24)], "CreamCeramic", segments=32),
         lathe("PotSoil", [(14.8, 24), (14.8, 24.5)], "DarkWood", segments=32)]
for stem in range(9):
    angle = stem * math.tau / 9
    dx, dy = math.cos(angle), math.sin(angle)
    points = [(dx * t * 30, dy * t * 30, 24 + 48 * math.sin(t * math.pi * 0.8)) for t in (0, 0.25, 0.5, 0.75, 1)]
    parts.append(line("FernStem", points, 0.45, "MoonleafGreen"))
    for tier in range(1, 5):
        t = tier / 5
        for side in (-1, 1):
            parts.append(leaf("FernLeaflet", (dx * t * 30, dy * t * 30, 24 + 48 * math.sin(t * math.pi * 0.8)),
                              9 * (1 - t) + 3, 1.6, angle + side * 0.95))
fern = export_asset("SM_PottedFern", parts, [((0, 0, 14), (36, 36, 28))])

parts = [box("PriceCardFrame", (0, 0, 8), (29, 4, 16), "DarkWood", 0.7),
         box("PriceCard", (0, -2.3, 8), (26, 0.4, 13), "CreamLabel", 0.3),
         text("DAWN TONIC", 2.5, (0, -2.6, 10)),
         text("12 BITS", 3, (0, -2.6, 4)),
         box("PriceCardFoot", (0, 0, 1), (30, 10, 2), "HoneyWood", 0.5)]
price_card = export_asset("SM_DawnPriceCard", parts, [((0, 0, 8), (30, 10, 16))])

parts = [box("InkPriceFrame", (0, 0, 8), (29, 4, 16), "BookCloth", 0.7),
         box("InkPriceCard", (0, -2.3, 8), (26, 0.4, 13), "CreamLabel", 0.3),
         text("MOONLIT INK", 2.5, (0, -2.6, 10)), text("18 BITS", 3, (0, -2.6, 4)),
         box("InkPriceFoot", (0, 0, 1), (30, 10, 2), "HoneyWood", 0.5)]
ink_price_card = export_asset("SM_InkPriceCard", parts, [((0, 0, 8), (30, 10, 16))])

parts = [box("InkRecipePages", (0, 0, 2), (40, 30, 3), "CreamLabel", 0.6),
         box("InkBookCover", (0, 0, 0.6), (43, 33, 1.2), "BookCloth", 0.7)]
for body, size, position in [("MOONLIT INK", 2.8, (0, 8, 3.65)),
                             ("1 MOONLEAF", 2.4, (0, 1, 3.65)),
                             ("2 SPRING WATER", 2, (0, -5, 3.65))]:
    lettering = text(body, size, position)
    lettering.rotation_euler = (0, 0, 0)
    parts.append(lettering)
ink_book = export_asset("SM_InkRecipeBook", parts, [((0, 0, 2), (43, 33, 4))])

parts = [box("DoorPost", (side * 104, 0, 100), (12, 20, 200), "DarkWood", 1) for side in (-1, 1)]
parts.append(box("DoorLintel", (0, 0, 211), (230, 25, 22), "HoneyWood", 1.2))
door = export_asset("SM_ShopDoorSurround", parts, [((-104, 0, 100), (12, 20, 200)),
                                                 ((104, 0, 100), (12, 20, 200)), ((0, 0, 211), (230, 25, 22))])

stock_ring = export_asset("SM_StockMarkerRing", [line("StockPositionRing",
    [(9 * math.cos(i * math.tau / 48), 9 * math.sin(i * math.tau / 48), 0.3) for i in range(49)], 0.3, "Brass")], [])

(OUTPUT / "manifest.json").write_text(json.dumps(dict(blender=bpy.app.version_string, units="cm", assets=assets, palette=PALETTE), indent=2))

# Source scene includes a simple studio arrangement for visual review.
bench.location = (-70, 0, 0)
shelf.location = (115, 24, 0)
bottle.location = (-65, -15, 100)
for asset in (bench, shelf, bottle):
    asset.rotation_euler[2] = math.pi
bottle.scale = (1, 1, 1)
for asset, position in [(brewer, (-95, 6, 100)), (jar, (-145, 5, 100)),
                        (jug, (-30, 12, 100)), (mortar, (-45, -24, 100)), (lamp, (-75, 10, 220)),
                        (till, (0, -23, 100))]:
    asset.location = position
    asset.rotation_euler[2] = math.pi
for asset, position in [(ink_bottle, (150, 17, 140)), (ink_price_card, (115, -4, 60)), (ink_book, (-5, 0, 100))]:
    asset.location = position
    asset.rotation_euler[2] = math.pi
for asset, position in [(crate, (-230, 30, 0)), (floor_tile, (-80, -135, 0)),
                        (wall_panel, (-70, 100, 0)), (wall_post, (-120, 90, 0)),
                        (window, (-70, 80, 105)), (book, (-15, 25, 100)),
                        (herbs, (35, 80, 150)), (fern, (205, 0, 0)), (price_card, (115, -4, 140))]:
    asset.location = position
    asset.rotation_euler[2] = math.pi
door.location = (370, 50, 0)
door.hide_render = True
stock_ring.location = (115, 17, 60)
for x in (85, 115, 145):
    duplicate = bpy.data.objects.new("BottleDisplay", bottle.data)
    scene.collection.objects.link(duplicate)
    duplicate.location = (x, 17, 100)
    duplicate.rotation_euler[2] = math.pi
    duplicate.scale = (1, 1, 1)
floor_mat = bpy.data.materials.new("StudioCream")
floor_mat.diffuse_color = (0.29, 0.25, 0.18, 1)
bpy.ops.mesh.primitive_plane_add(size=2000)
bpy.context.object.location.z = -3.2
bpy.context.object.data.materials.append(floor_mat)
for position, energy, size, color in [((-180, -220, 400), 3500, 300, (1, 0.79, 0.57)), ((180, 120, 280), 2800, 220, (0.65, 0.8, 1))]:
    bpy.ops.object.light_add(type="AREA", location=position)
    light = bpy.context.object
    # Blender energy is adjusted for the centimetre modelling coordinate space.
    light.data.energy = energy * 400
    light.data.shape = "DISK"
    light.data.size = size
    light.data.color = color
    light.rotation_euler = (Vector((0, 0, 90)) - light.location).to_track_quat("-Z", "Y").to_euler()
bpy.ops.object.camera_add(location=(350, -530, 290))
camera = bpy.context.object
camera.rotation_euler = (Vector((0, 0, 125)) - camera.location).to_track_quat("-Z", "Y").to_euler()
camera.data.type = "ORTHO"
camera.data.ortho_scale = 600
camera.data.clip_end = 5000
scene.camera = camera
scene.render.engine = "CYCLES"
scene.cycles.samples = 32
scene.render.resolution_x = 1500
scene.render.resolution_y = 1000
scene.render.resolution_percentage = 100
scene.world = bpy.data.worlds.new("StudioWorld")
scene.world.use_nodes = True
scene.world.node_tree.nodes["Background"].inputs[0].default_value = (0.3, 0.36, 0.45, 1)
scene.world.node_tree.nodes["Background"].inputs[1].default_value = 0.4
scene.render.filepath = str(OUTPUT / "shop-kit-preview.png")
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "shop-corner.blend"))
bpy.ops.render.render(write_still=True)
for obj in scene.objects:
    if obj.type == "MESH" and obj != bottle:
        obj.hide_render = True
camera.location = (-35, -80, 146)
camera.rotation_euler = (Vector((-65, -15, 121)) - camera.location).to_track_quat("-Z", "Y").to_euler()
camera.data.ortho_scale = 32
scene.render.resolution_x = 900
scene.render.resolution_y = 1100
scene.render.filepath = str(OUTPUT / "bottle-fit-preview.png")
bpy.ops.render.render(write_still=True)
print("SHOP_MODELS_COMPLETE " + json.dumps(assets))
