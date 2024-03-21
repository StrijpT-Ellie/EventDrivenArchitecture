// Import the Three.js library
import * as THREE from 'https://threejs.org/build/three.module.js';

//Global cube variable
var cube;

// Create a scene
var scene = new THREE.Scene();

// Create a camera
var camera = new THREE.PerspectiveCamera(75, window.innerWidth/window.innerHeight, 0.1, 1000);
camera.position.z = 10;

// Create a renderer
var renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

// Create a geometry
var geometry = new THREE.BoxGeometry(1, 1, 1);

// Create a material
var material = new THREE.MeshBasicMaterial({color: 0x00ff00});

// Create a cube
cube = new THREE.Mesh(geometry, material);

// Add the cube to the scene
scene.add(cube);

// Define the change in each direction
var dx = (Math.random() - 0.5) / 10;
var dy = (Math.random() - 0.5) / 10;
var dz = (Math.random() - 0.5) / 10;

// Create a BufferGeometry for the particles
var particles = new THREE.BufferGeometry();
var positions = [];

// Create particles
for (var i = 0; i < 1000; i++) {
    positions.push(
        (Math.random() - 0.5) * 10,  // x
        (Math.random() - 0.5) * 10,  // y
        (Math.random() - 0.5) * 10   // z
    );
}

// Add positions to the geometry
particles.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));

// Create a material for the particles
var particleMaterial = new THREE.PointsMaterial({
    color: 0xffffff,
    size: 0.01
});

// Create a particle system
var particleSystem = new THREE.Points(particles, particleMaterial);

// Add the particle system to the cube
cube.add(particleSystem);

// Animation loop
function animate() {
    requestAnimationFrame(animate);

    // Rotate the cube
    cube.rotation.x += 0.01;
    cube.rotation.y += 0.01;

    // Update the cube's position
    cube.position.x += dx;
    cube.position.y += dy;
    cube.position.z += dz;

    // If the cube's position exceeds a certain limit, reverse the direction
    if (Math.abs(cube.position.x) > 5) dx = -dx;
    if (Math.abs(cube.position.y) > 5) dy = -dy;
    if (Math.abs(cube.position.z) > 5) dz = -dz;

    // Update the particles
    var positions = particleSystem.geometry.attributes.position.array;
    for (var i = 0; i < positions.length; i += 3) {
        positions[i] += (Math.random() - 0.5) / 10;
        positions[i + 1] += (Math.random() - 0.5) / 10;
        positions[i + 2] += (Math.random() - 0.5) / 10;
    }
    particleSystem.geometry.attributes.position.needsUpdate = true;

    // Render the scene
    renderer.render(scene, camera);
}

// Start the animation loop
animate();


// Function to divide a cube into two
function divideCube() {
    console.log("divideCube function called");

    // Check if cube is properly initialized
    if (!cube) {
        console.error("Cube is not properly initialized");
        return;
    }

    // Check if cube has geometry
    if (!cube.geometry) {
        console.error("Cube does not have geometry");
        return;
    }

    // Get the size and position of the existing cube
    var sizeX = cube.geometry.parameters.width;
    var sizeY = cube.geometry.parameters.height;
    var sizeZ = cube.geometry.parameters.depth;
    var positionX = cube.position.x;
    var positionY = cube.position.y;
    var positionZ = cube.position.z;

    // Create a new cube with the same size and position
    var geometry = new THREE.BoxGeometry(sizeX, sizeY, sizeZ);
    var material = new THREE.MeshBasicMaterial({color: 0x00ff00}); // Change color as needed
    var newCube = new THREE.Mesh(geometry, material);
    newCube.position.set(positionX, positionY, positionZ);

    // Add the new cube to the scene
    scene.add(newCube);
}

// Set up an EventSource to listen for server-sent events
var source = new EventSource("/stream");

// When an event is received, call the divideCube function
source.onmessage = function(event) {
    console.log("Received event with data:", event.data);
    if (event.data === "divide") {
        divideCube();
        // Reset the event.data to prevent continuous division
        event.data === null;
    }
};

// Create a group to hold the cubes (LEDs)
var cubeGroup = new THREE.Group();

// Function to create a single sphere (ball) and a cube
function createGrid() {
    // Remove all previous children from the group
    while(cubeGroup.children.length > 0){ 
        cubeGroup.remove(cubeGroup.children[0]); 
    }

    // Create a new sphere (ball)
    var geometry = new THREE.SphereGeometry(1, 32, 32);
    var material = new THREE.MeshBasicMaterial({color: 0x8000ff}); // Change color as needed
    var sphere = new THREE.Mesh(geometry, material);

    // Add the sphere to the group
    cubeGroup.add(sphere);

    // Store the sphere in a global variable for collision detection
    window.sphere = sphere;

    // Create a cube
    var cubeGeometry = new THREE.BoxGeometry(1, 1, 1);
    var cubeMaterial = new THREE.MeshBasicMaterial({color: 0x00ff00}); // Change color as needed
    var cube = new THREE.Mesh(cubeGeometry, cubeMaterial);

    // Add the cube to the group
    cubeGroup.add(cube);

    // Store the cube in a global variable for collision detection
    window.cube = cube;

    // Add the group to the scene
    scene.add(cubeGroup);
}

// Flag to track whether a collision has occurred
var collisionDetected = false;

// Function to check for collision between the cube and the sphere
function checkCollision(cube, sphere) {
    cube.geometry.computeBoundingSphere();
    sphere.geometry.computeBoundingSphere();

    var cubeRadius = cube.geometry.boundingSphere.radius;
    var sphereRadius = sphere.geometry.boundingSphere.radius;

    var distance = cube.position.distanceTo(sphere.position);

    if (distance < cubeRadius + sphereRadius) {
        // Collision detected
        if (!collisionDetected) {
            console.log("Collision detected!");
            collisionDetected = true;

            // Change the color of the cube and sphere
            cube.material.color.set(0xff0000); // Change to red
            sphere.material.color.set(0xff0000); // Change to red
        }

        // Calculate the direction from the sphere to the cube
        var direction = new THREE.Vector3().subVectors(cube.position, sphere.position).normalize();

        // Add a force that pushes the cube away from the sphere
        cube.position.add(direction.multiplyScalar(0.1));
    } else {
        // No collision
        if (collisionDetected) {
            collisionDetected = false;

            // Change the color of the cube and sphere back to their original colors
            cube.material.color.set(0x00ff00); // Change back to green
            sphere.material.color.set(0x0000ff); // Change back to blue
        }
    }
}

// Define the sphere
var geometry = new THREE.SphereGeometry(1, 32, 32);
var material = new THREE.MeshBasicMaterial({color: 0xffff00});
var sphere = new THREE.Mesh(geometry, material);
scene.add(sphere);

// Animation loop
function animateSphere() {
    requestAnimationFrame(animateSphere);

    // Update the cube's position
    cube.position.x += dx;
    cube.position.y += dy;
    cube.position.z += dz;

    // If the cube's position exceeds a certain limit, reverse the direction
    if (Math.abs(cube.position.x) > 5) dx = -dx;
    if (Math.abs(cube.position.y) > 5) dy = -dy;
    if (Math.abs(cube.position.z) > 5) dz = -dz;

    // Check for collision
    checkCollision(cube, sphere);

    // Render the scene
    renderer.render(scene, camera);
}

animateSphere();

// Set up an EventSource to listen for server-sent events
var source = new EventSource("/stream");

// When an event is received, call the createGrid function
source.onmessage = function(event) {
    console.log("Received event with data:", event.data);
    if (event.data === "createGrid") {
        createGrid();
        // Check the value of the 'cube' and 'sphere' objects
        console.log("Cube object: ", window.cube);
        console.log("Sphere object: ", window.sphere);
        checkCollision(window.cube, window.sphere);
    }
};