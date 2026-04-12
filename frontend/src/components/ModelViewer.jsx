import { Canvas } from '@react-three/fiber'
import { useGLTF, OrbitControls, Bounds, Environment } from '@react-three/drei'
import { Suspense, useMemo } from 'react'

function Model({ url }) {
  const { scene } = useGLTF(url)
  const cloned = useMemo(() => scene.clone(true), [scene])
  return <primitive object={cloned} />
}

export default function ModelViewer({ url }) {
  return (
    <Canvas
      style={{ height: 260, width: '100%' }}
      dpr={[1, 2]}
      gl={{ antialias: true }}
    >
      <ambientLight intensity={0.8} />
      <directionalLight position={[5, 10, 5]} intensity={1.5} />
      <pointLight position={[-4, -4, -4]} intensity={0.4} color="#b06bff" />
      <Suspense fallback={null}>
        <Bounds fit clip observe>
          <Model url={url} />
        </Bounds>
        <Environment preset="city" />
        <OrbitControls
          autoRotate
          autoRotateSpeed={1.2}
          enableZoom={false}
          enablePan={false}
          makeDefault
        />
      </Suspense>
    </Canvas>
  )
}
