// This script uses shaders to create an inimated cube

// Define vertex shader code
const vertexShaderSource = `
  attribute vec4 a_position;
  uniform mat4 u_projection;
  void main() {
    gl_Position = u_projection * a_position;
  }
`;

// Define fragment shader code
const fragmentShaderSource = `
  precision mediump float;
  uniform vec4 u_color;
  void main() {
    gl_FragColor = u_color;
  }
`;

// Create WebGL context
const canvas = document.getElementById('glCanvas');
const gl = canvas.getContext('webgl');
if (!gl) {
  console.error('Unable to initialize WebGL. Your browser may not support it.');
}

// Compile shader
function compileShader(gl, shaderSource, shaderType) {
  const shader = gl.createShader(shaderType);
  gl.shaderSource(shader, shaderSource);
  gl.compileShader(shader);
  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    console.error('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(shader));
    gl.deleteShader(shader);
    return null;
  }
  return shader;
}

// Create shader program
function createShaderProgram(gl, vertexShaderSource, fragmentShaderSource) {
  const vertexShader = compileShader(gl, vertexShaderSource, gl.VERTEX_SHADER);
  const fragmentShader = compileShader(gl, fragmentShaderSource, gl.FRAGMENT_SHADER);
  
  const shaderProgram = gl.createProgram();
  gl.attachShader(shaderProgram, vertexShader);
  gl.attachShader(shaderProgram, fragmentShader);
  gl.linkProgram(shaderProgram);
  
  if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
    console.error('Unable to initialize the shader program: ' + gl.getProgramInfoLog(shaderProgram));
    return null;
  }
  
  return shaderProgram;
}

// Create a square
const positionBuffer = gl.createBuffer();
gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
const size = 3.0;  // Change this value to change the size of the square
const positions = [
  size,  size,
 -size,  size,
  size, -size,
 -size, -size,
];
gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);

// Create shader program
const shaderProgram = createShaderProgram(gl, vertexShaderSource, fragmentShaderSource);
const programInfo = {
  program: shaderProgram,
  attribLocations: {
    vertexPosition: gl.getAttribLocation(shaderProgram, 'a_position'),
  },
  uniformLocations: {
    projectionMatrix: gl.getUniformLocation(shaderProgram, 'u_projection'),
    color: gl.getUniformLocation(shaderProgram, 'u_color'),
  },
};

// Define a particle
class Particle {
  constructor(position, velocity, lifespan) {
    this.position = { ...position };
    this.velocity = { ...velocity };
    this.lifespan = lifespan;
  }

  update() {
    this.position.x += this.velocity.x;
    this.position.y += this.velocity.y;
    this.lifespan--;
  }

  isDead() {
    return this.lifespan <= 0;
  }
}

// Define a particle system
class ParticleSystem {
  constructor() {
    this.particles = [];
  }

  addParticle(position, velocity) {
    const lifespan = 100;  // Adjust this value to change the lifespan of the particles
    this.particles.push(new Particle(position, velocity, lifespan));
  }

  update() {
    for (let i = this.particles.length - 1; i >= 0; i--) {
      const p = this.particles[i];
      p.update();
      if (p.isDead()) {
        this.particles.splice(i, 1);
      }
    }
  }
}

// Create a particle system
const particleSystem = new ParticleSystem();

// Define the square's position and velocity
let squarePosition = { x: 0, y: 0 };
let squareVelocity = { x: (Math.random() - 0.5) / 100, y: (Math.random() - 0.5) / 100 };

// Draw the scene
function drawScene(gl, programInfo) {
  gl.clearColor(0.0, 0.0, 0.0, 1.0);  // Clear to black
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

  // Update the square's position
  squarePosition.x += squareVelocity.x;
  squarePosition.y += squareVelocity.y;

  // Check if the square has hit the wall
  if (Math.abs(squarePosition.x) > 1) {
    squareVelocity.x = -squareVelocity.x;
    squareVelocity.x += (Math.random() - 0.5) / 100;  // Add some randomness to the velocity
  }
  if (Math.abs(squarePosition.y) > 1) {
    squareVelocity.y = -squareVelocity.y;
    squareVelocity.y += (Math.random() - 0.5) / 100;  // Add some randomness to the velocity
  }

  // Create a translation matrix to move the square
  const translationMatrix = mat4.create();
  mat4.translate(translationMatrix, translationMatrix, [squarePosition.x, squarePosition.y, 0]);

  // Create the projection matrix
  const projectionMatrix = mat4.create();
  mat4.ortho(projectionMatrix, -6, 6, -6, 6, -1, 1);
  mat4.multiply(projectionMatrix, projectionMatrix, translationMatrix);

  gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
  gl.vertexAttribPointer(programInfo.attribLocations.vertexPosition, 2, gl.FLOAT, false, 0, 0);
  gl.enableVertexAttribArray(programInfo.attribLocations.vertexPosition);

  gl.useProgram(programInfo.program);
  gl.uniformMatrix4fv(programInfo.uniformLocations.projectionMatrix, false, projectionMatrix);
  gl.uniform4fv(programInfo.uniformLocations.color, [1.0, 0.0, 0.0, 1.0]);  // Red color

  gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);

  // Request the next frame
  requestAnimationFrame(() => drawScene(gl, programInfo));
}

// Start the animation
drawScene(gl, programInfo);