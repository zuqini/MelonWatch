-- Simple Scene:
-- An extremely simple scene that will render right out of the box
-- with the provided skeleton code.  It doesn't rely on hierarchical
-- transformations.

-- Create the top level root node named 'root'.
rootNode = gr.node('root')

-- Create a GeometryNode with MeshId = 'cube', and name = 'torso'.
-- MeshId's must reference a .obj file that has been previously loaded into
-- the MeshConsolidator instance within A3::init().
barrel = gr.mesh('cube', 'barrel')
barrel:scale(0.01, 0.01, 0.2)
barrel:translate(0.05, -0.05, -0.22)
barrel:set_material(gr.material({0.2, 0.2, 0.2}, {0.8, 0.8, 0.8}, 10.0))
rootNode:add_child(barrel)

silencer = gr.mesh('cube', 'silencer')
silencer:scale(0.008, 0.008, 0.2)
silencer:translate(0.05, -0.05, -0.3)
silencer:set_material(gr.material({0.1, 0.1, 0.1}, {0.8, 0.8, 0.8}, 10.0))
rootNode:add_child(silencer)

handleBack = gr.mesh('cube', 'handleBack')
handleBack:scale(0.009, 0.03, 0.01)
handleBack:rotate('x', -20.0)
handleBack:translate(0.05, -0.065, -0.15)
handleBack:set_material(gr.material({0.1, 0.1, 0.1}, {0.8, 0.8, 0.8}, 10.0))
rootNode:add_child(handleBack);

magazine = gr.mesh('cube', 'magazine')
magazine:scale(0.009, 0.05, 0.01)
magazine:rotate('x', 10.0)
magazine:translate(0.05, -0.075, -0.2)
magazine:set_material(gr.material({0.1, 0.1, 0.1}, {0.8, 0.8, 0.8}, 10.0))
rootNode:add_child(magazine);

handleFront = gr.mesh('cube', 'handleFront')
handleFront:scale(0.009, 0.025, 0.01)
handleFront:translate(0.05, -0.065, -0.27)
handleFront:set_material(gr.material({0.1, 0.1, 0.1}, {0.8, 0.8, 0.8}, 10.0))
rootNode:add_child(handleFront);

scopeFrame = gr.mesh('cube', 'scopeFrame')
scopeFrame:scale(0.01, 0.01, 0.06)
scopeFrame:translate(0.05, -0.04, -0.18)
scopeFrame:set_material(gr.material({0.1, 0.1, 0.1}, {0.8, 0.8, 0.8}, 10.0))
rootNode:add_child(scopeFrame);

scopeLens = gr.mesh('cube', 'scopeLens')
scopeLens:scale(0.005, 0.005, 0.065)
scopeLens:translate(0.05, -0.04, -0.18)
scopeLens:set_material(gr.material({0, 1, 1}, {0.8, 0.8, 0.8}, 0))
rootNode:add_child(scopeLens);

sightFront = gr.mesh('cube', 'sightFront')
sightFront:scale(0.002, 0.005, 0.0002)
sightFront:translate(0.05, -0.043, -0.3)
sightFront:set_material(gr.material({0.1, 0.1, 0.1}, {0.8, 0.8, 0.8}, 10.0))
rootNode:add_child(sightFront);

stock = gr.mesh('cube', 'stock')
stock:scale(0.009, 0.015, 0.005)
stock:translate(0.05, -0.053, -0.12)
stock:set_material(gr.material({0.1, 0.1, 0.1}, {0.8, 0.8, 0.8}, 10.0))
rootNode:add_child(stock);

-- Return the root with all of it's childern.  The SceneNode A3::m_rootNode will be set
-- equal to the return value from this Lua script.
return rootNode
