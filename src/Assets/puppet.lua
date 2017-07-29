-- puppet.lua
-- A simplified puppet without posable joints, but that
-- looks roughly humanoid.

rootnode = gr.node('root')
rootnode:rotate('y', -20.0)
rootnode:scale( 0.25, 0.25, 0.25 )
rootnode:translate(0.0, 0.0, -1.0)

red = gr.material({1.0, 0.0, 0.0}, {0.1, 0.1, 0.1}, 10)
blue = gr.material({0.0, 0.0, 1.0}, {0.1, 0.1, 0.1}, 10)
green = gr.material({0.0, 1.0, 0.0}, {0.1, 0.1, 0.1}, 10)
white = gr.material({1.0, 1.0, 1.0}, {0.1, 0.1, 0.1}, 10)

torso = gr.mesh('cube', 'torso')
rootnode:add_child(torso)
torso:set_material(white)
torso:scale(0.5,1.0,0.5);

head = gr.mesh('cube', 'head')
torso:add_child(head)
head:scale(1.0/0.5, 1.0, 1.0/0.5)
head:scale(0.4, 0.4, 0.4)
head:translate(0.0, 0.9, 0.0)
head:set_material(red)

neck = gr.mesh('sphere', 'neck')
torso:add_child(neck)
neck:scale(1.0/0.5, 1.0, 1.0/0.5)
neck:scale(0.15, 0.3, 0.15)
neck:translate(0.0, 0.6, 0.0)
neck:set_material(blue)

ears = gr.mesh('sphere', 'ears')
head:add_child(ears)
ears:scale(1.2, 0.08, 0.08)
ears:set_material(red)
ears:set_material(blue)

leftEye = gr.mesh('cube', 'leftEye')
head:add_child(leftEye)
leftEye:scale(0.2, 0.1, 0.1)
leftEye:translate(-0.2, 0.2, 0.5)
leftEye:set_material(blue)

rightEye = gr.mesh('cube', 'rightEye')
head:add_child(rightEye)
rightEye:scale(0.2, 0.1, 0.1)
rightEye:translate(0.2, 0.2, 0.5)
rightEye:set_material(blue)

leftShoulder = gr.mesh('sphere', 'leftShoulder')
torso:add_child(leftShoulder)
leftShoulder:scale(1/0.5,1.0,1/0.5);
leftShoulder:scale(0.2, 0.2, 0.2)
leftShoulder:translate(-0.4, 0.35, 0.0)
leftShoulder:set_material(blue)

leftArm = gr.mesh('cube', 'leftArm')
torso:add_child(leftArm)
leftArm:scale(1/0.5, 1.0, 1/0.5);
leftArm:scale(0.4, 0.1, 0.1)
leftArm:rotate('z', 50);
leftArm:translate(-0.8, 0.0, 0.0)
leftArm:set_material(red)

rightShoulder = gr.mesh('sphere', 'rightShoulder')
torso:add_child(rightShoulder)
rightShoulder:scale(1/0.5,1.0,1/0.5);
rightShoulder:scale(0.2, 0.2, 0.2)
rightShoulder:translate(0.4, 0.35, 0.0)
rightShoulder:set_material(blue)

rightArm = gr.mesh('cube', 'rightArm')
torso:add_child(rightArm)
rightArm:scale(1/0.5,1.0,1/0.5);
rightArm:scale(0.4, 0.1, 0.1)
rightArm:rotate('z', -50);
rightArm:translate(0.8, 0.0, 0.0)
rightArm:set_material(red)

leftHip = gr.mesh('sphere', 'leftHip')
torso:add_child(leftHip)
leftHip:scale(1/0.5,1.0,1/0.5);
leftHip:scale(0.21, 0.21, 0.21)
leftHip:translate(-0.38, -0.5, 0.0)
leftHip:set_material(blue)

rightHip = gr.mesh('sphere', 'rightHip')
torso:add_child(rightHip)
rightHip:scale(1/0.5,1.0,1/0.5);
rightHip:scale(0.21, 0.21, 0.21)
rightHip:translate(0.38, -0.5, 0.0)
rightHip:set_material(blue)

leftLeg = gr.mesh('cube', 'leftLeg')
leftHip:add_child(leftLeg)
leftLeg:scale(0.5,4,0.5)
leftLeg:translate(0,-2.8,0)
leftLeg:set_material(red)

rightLeg = gr.mesh('cube', 'rightLeg')
rightHip:add_child(rightLeg)
rightLeg:scale(0.5,4,0.5)
rightLeg:translate(0,-2.8,0)
rightLeg:set_material(red)


return rootnode
